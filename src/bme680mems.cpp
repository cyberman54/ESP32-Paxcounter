#ifdef HAS_BME

#include "bme680mems.h"

// Local logging tag
static const char TAG[] = "main";

bmeStatus_t bme_status;
TaskHandle_t BmeTask;

// initialize BME680 sensor
int bme_init(void) {

  // struct bme680_dev gas_sensor;
  Wire.begin(HAS_BME, 400000); // I2C connect to BME680 sensor with 400 KHz

  // Call to the function which initializes the BSEC library
  // Switch on low-power mode and provide no temperature offset

  return_values_init ret =
      bsec_iot_init(BSEC_SAMPLE_RATE_LP, 0.0f, i2c_write, i2c_read,
                    user_delay_ms, state_load, config_load);

  if ((int)ret.bme680_status) {
    ESP_LOGE(TAG, "Could not initialize BME680, error %d", (int)ret.bme680_status);
  } else if ((int)ret.bsec_status) {
    ESP_LOGE(TAG, "Could not initialize BSEC library, error %d", (int)ret.bsec_status);
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
  bme_status.pressure = pressure;
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

/*!
 * @brief           Load library config from non-volatile memory
 *
 * @param[in,out]   config_buffer    buffer to hold the loaded state string
 * @param[in]       n_buffer        size of the allocated state buffer
 *
 * @return          number of bytes copied to config_buffer
 */
uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer) {
  // ...
  // Load a library config from non-volatile memory, if available.
  //
  // Return zero if loading was unsuccessful or no config was available,
  // otherwise return length of loaded config string.
  // ...
  return 0;
}

/*!
 * @brief           Interrupt handler for press of a ULP plus button
 *
 * @return          none
 */

void user_delay_ms(uint32_t period) { vTaskDelay(period / portTICK_PERIOD_MS); }

int64_t get_timestamp_us() { return (int64_t)millis() * 1000; }

#endif // HAS_BME