#ifdef HAS_BME

#include "bme680mems.h"

// Local logging tag
static const char TAG[] = "main";

#define NUM_USED_OUTPUTS 8

bmeStatus_t bme_status;

// initialize BME680 sensor
int bme_init(void) {

  return_values_init ret = {BME680_OK, BSEC_OK};
  struct bme680_dev gas_sensor;
  Wire.begin(HAS_BME, 400000); // I2C connect to BME680 sensor with 400 KHz

  /* Call to the function which initializes the BSEC library
   * Switch on low-power mode and provide no temperature offset */
  ret = bsec_iot_init(BSEC_SAMPLE_RATE_LP, 0.0f, user_i2c_write, user_i2c_read,
                      user_delay_ms, state_load, config_load);
  if (ret.bme680_status) {
    /* Could not intialize BME680 */
    return (int)ret.bme680_status;
  } else if (ret.bsec_status) {
    /* Could not intialize BSEC library */
    return (int)ret.bsec_status;
  }
}

bool bme_read(void) {
  /*

    bool ret = bme.performReading();
    if (ret) {
      // read current BME data and buffer in global struct
      bme_status.temperature = bme.temperature;
      bme_status.pressure = (uint16_t)(bme.pressure / 100.0); // convert Pa ->
    hPa bme_status.humidity = bme.humidity; bme_status.gas_resistance =
    (uint16_t)(bme.gas_resistance / 1000.0); // convert Ohm -> kOhm
      ESP_LOGI(TAG, "BME680 sensor data read success");
    } else {
      ESP_LOGI(TAG, "BME680 sensor read error");
    }
    return ret;

  */
}

// loop function which reads and processes data based on sensor settings
void bme_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

#ifdef HAS_BME

  // State is saved every 10.000 samples, which means every 10.000 * 3 secs =
  // 500 minutes
  bsec_iot_loop(sleep, get_timestamp_us, output_ready, state_save, 10000);

  vTaskDelete(NULL); // shoud never be reached

#endif

} // bme_loop()

int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,
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

int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,
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

void ulp_plus_button_press() {
  /* We call bsec_update_subscription() in order to instruct BSEC to perform an
   * extra measurement at the next possible time slot
   */

  bsec_sensor_configuration_t requested_virtual_sensors[1];
  uint8_t n_requested_virtual_sensors = 1;
  bsec_sensor_configuration_t
      required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
  uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;
  bsec_library_return_t status = BSEC_OK;

  /* To trigger a ULP plus, we request the IAQ virtual sensor with a specific
   * sample rate code */
  requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_IAQ;
  requested_virtual_sensors[0].sample_rate =
      BSEC_SAMPLE_RATE_ULP_MEASUREMENT_ON_DEMAND;

  /* Call bsec_update_subscription() to enable/disable the requested virtual
   * sensors */
  status = bsec_update_subscription(
      requested_virtual_sensors, n_requested_virtual_sensors,
      required_sensor_settings, &n_required_sensor_settings);

  /* The status code would tell is if the request was accepted. It will be
   * rejected if the sensor is not already in ULP mode, or if the time
   * difference between requests is too short, for example. */
}

void user_delay_ms(uint32_t period) { vTaskDelay(period / portTICK_PERIOD_MS); }

int64_t get_timestamp_us() { return (int64_t)millis() * 1000; }

#endif // HAS_BME