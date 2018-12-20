#ifdef HAS_BME

#include "bme680mems.h"

// Local logging tag
static const char TAG[] = "main";

bmeStatus_t bme_status;
TaskHandle_t BmeTask;

float bme_offset = (float)BME_TEMP_OFFSET;

// --- Bosch BSEC library configuration ---
// 3,3V supply voltage; 3s max time between sensor_control calls; 4 days
// calibration. Change this const if not applicable for your application (see
// BME680 datasheet)
const uint8_t bsec_config_iaq[454] = {
    1,   7,   4,   1,   61,  0,   0,   0,   0,   0,   0,   0,   174, 1,   0,
    0,   48,  0,   1,   0,   137, 65,  0,   63,  205, 204, 204, 62,  0,   0,
    64,  63,  205, 204, 204, 62,  0,   0,   225, 68,  0,   192, 168, 71,  64,
    49,  119, 76,  0,   0,   0,   0,   0,   80,  5,   95,  0,   0,   0,   0,
    0,   0,   0,   0,   28,  0,   2,   0,   0,   244, 1,   225, 0,   25,  0,
    0,   128, 64,  0,   0,   32,  65,  144, 1,   0,   0,   112, 65,  0,   0,
    0,   63,  16,  0,   3,   0,   10,  215, 163, 60,  10,  215, 35,  59,  10,
    215, 35,  59,  9,   0,   5,   0,   0,   0,   0,   0,   1,   88,  0,   9,
    0,   229, 208, 34,  62,  0,   0,   0,   0,   0,   0,   0,   0,   218, 27,
    156, 62,  225, 11,  67,  64,  0,   0,   160, 64,  0,   0,   0,   0,   0,
    0,   0,   0,   94,  75,  72,  189, 93,  254, 159, 64,  66,  62,  160, 191,
    0,   0,   0,   0,   0,   0,   0,   0,   33,  31,  180, 190, 138, 176, 97,
    64,  65,  241, 99,  190, 0,   0,   0,   0,   0,   0,   0,   0,   167, 121,
    71,  61,  165, 189, 41,  192, 184, 30,  189, 64,  12,  0,   10,  0,   0,
    0,   0,   0,   0,   0,   0,   0,   229, 0,   254, 0,   2,   1,   5,   48,
    117, 100, 0,   44,  1,   112, 23,  151, 7,   132, 3,   197, 0,   92,  4,
    144, 1,   64,  1,   64,  1,   144, 1,   48,  117, 48,  117, 48,  117, 48,
    117, 100, 0,   100, 0,   100, 0,   48,  117, 48,  117, 48,  117, 100, 0,
    100, 0,   48,  117, 48,  117, 100, 0,   100, 0,   100, 0,   100, 0,   48,
    117, 48,  117, 48,  117, 100, 0,   100, 0,   100, 0,   48,  117, 48,  117,
    100, 0,   100, 0,   44,  1,   44,  1,   44,  1,   44,  1,   44,  1,   44,
    1,   44,  1,   44,  1,   44,  1,   44,  1,   44,  1,   44,  1,   44,  1,
    44,  1,   8,   7,   8,   7,   8,   7,   8,   7,   8,   7,   8,   7,   8,
    7,   8,   7,   8,   7,   8,   7,   8,   7,   8,   7,   8,   7,   8,   7,
    112, 23,  112, 23,  112, 23,  112, 23,  112, 23,  112, 23,  112, 23,  112,
    23,  112, 23,  112, 23,  112, 23,  112, 23,  112, 23,  112, 23,  255, 255,
    255, 255, 255, 255, 255, 255, 220, 5,   220, 5,   220, 5,   255, 255, 255,
    255, 255, 255, 220, 5,   220, 5,   255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 44,  1,   0,   0,   0,   0,
    239, 79,  0,   0};

// initialize BME680 sensor
int bme_init(void) {

  // struct bme680_dev gas_sensor;
  Wire.begin(HAS_BME, 400000); // I2C connect to BME680 sensor with 400 KHz

  // Call to the function which initializes the BSEC library
  // Switch on low-power mode and provide no temperature offset

  return_values_init ret =
      bsec_iot_init(BSEC_SAMPLE_RATE_LP, bme_offset, i2c_write, i2c_read,
                    user_delay_ms, state_load, config_load);

  if ((int)ret.bme680_status) {
    ESP_LOGE(TAG, "Could not initialize BME680, error %d",
             (int)ret.bme680_status);
  } else if ((int)ret.bsec_status) {
    ESP_LOGE(TAG, "Could not initialize BSEC library, error %d",
             (int)ret.bsec_status);
  } else {
    ESP_LOGI(TAG, "BME680 sensor found and initialized");
    return 1;
  }
  return 0;
}

void output_ready(int64_t timestamp, float iaq, uint8_t iaq_accuracy,
                  float temperature, float humidity, float pressure,
                  float raw_temperature, float raw_humidity, float gas,
                  bsec_library_return_t bsec_status, float static_iaq,
                  float co2_equivalent, float breath_voc_equivalent) {

  bme_status.temperature = temperature;
  bme_status.humidity = humidity;
  bme_status.pressure = (pressure / 100.0); // conversion Pa -> hPa
  bme_status.iaq = iaq;
}

// loop function which reads and processes data based on sensor settings
void bme_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

#ifdef HAS_BME
  // State is saved every 10.000 samples, which means every 10.000 * 3 secs =
  // 500 minutes
  bsec_iot_loop(user_delay_ms, get_timestamp_us, output_ready, state_save,
                10000);
#endif
  vTaskDelete(BmeTask); // should never be reached
} // bme_loop()

int8_t i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,
                uint16_t len) {
  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
  uint16_t i;

  Wire.beginTransmission(dev_id);
  Wire.write(reg_addr);
  rslt = Wire.endTransmission();

  Wire.requestFrom((int)dev_id, (int)len);
  for (i = 0; (i < len) && Wire.available(); i++) {
    reg_data[i] = Wire.read();
  }

  return rslt;
}

int8_t i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,
                 uint16_t len) {
  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
  uint16_t i;

  Wire.beginTransmission(dev_id);
  Wire.write(reg_addr);
  for (i = 0; i < len; i++) {
    Wire.write(reg_data[i]);
  }
  rslt = Wire.endTransmission();

  return rslt;
}

/*!
 * @brief           Load previous library state from non-volatile memory
 *
 * @param[in,out]   state_buffer    buffer to hold the loaded state string
 * @param[in]       n_buffer        size of the allocated state buffer
 *
 * @return          number of bytes copied to state_buffer
 */
uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer) {
  // ...
  // Load a previous library state from non-volatile memory, if available.
  //
  // Return zero if loading was unsuccessful or no state was available,
  // otherwise return length of loaded state string.
  // ...
  return 0;
}

/*!
 * @brief           Save library state to non-volatile memory
 *
 * @param[in]       state_buffer    buffer holding the state to be stored
 * @param[in]       length          length of the state string to be stored
 *
 * @return          none
 */
void state_save(const uint8_t *state_buffer, uint32_t length) {
  // ...
  // Save the string some form of non-volatile memory, if possible.
  // ...
}

uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer) {

  // Load a library config from non-volatile memory, if available.
  // Return zero if loading was unsuccessful or no config was available,
  // otherwise return length of loaded config string.

  memcpy(config_buffer, bsec_config_iaq, sizeof(bsec_config_iaq));
  return sizeof(bsec_config_iaq);
}

void user_delay_ms(uint32_t period) { delay(period); }

int64_t get_timestamp_us() { return (int64_t)millis() * 1000; }

#endif // HAS_BME