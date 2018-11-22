#ifdef HAS_BME

#include "bme680mems.h"

// Local logging tag
static const char TAG[] = "main";

bmeStatus_t bme_status;

void bme_init(void) {

  // initialize BME680 sensor

  struct bme680_dev gas_sensor;

  Wire.begin(HAS_BME, 400000); // I2C connect to BME680 sensor with 400 KHz

  // configure sensor for I2C communication
  gas_sensor.dev_id = BME_ADDR;
  gas_sensor.intf = BME680_I2C_INTF;
  gas_sensor.read = user_i2c_read;
  gas_sensor.write = user_i2c_write;
  gas_sensor.delay_ms = user_delay_ms;
  gas_sensor.amb_temp = 25;

  int8_t rslt = BME680_OK;
  rslt = bme680_init(&gas_sensor);

  if (rslt == BME680_OK) {
    ESP_LOGI(TAG, "BME680 sensor found");
  } else {
    ESP_LOGE(TAG, "BME680 sensor not found on i2c bus");
    return;
  }

  // configure BME680 sensor in forced mode

  uint8_t set_required_settings;

  /* Set the temperature, pressure and humidity settings */
  gas_sensor.tph_sett.os_hum = BME680_OS_2X;
  gas_sensor.tph_sett.os_pres = BME680_OS_4X;
  gas_sensor.tph_sett.os_temp = BME680_OS_8X;
  gas_sensor.tph_sett.filter = BME680_FILTER_SIZE_3;

  /* Set the remaining gas sensor settings and link the heating profile */
  gas_sensor.gas_sett.run_gas = BME680_ENABLE_GAS_MEAS;
  /* Create a ramp heat waveform in 3 steps */
  gas_sensor.gas_sett.heatr_temp = 320; /* degree Celsius */
  gas_sensor.gas_sett.heatr_dur = 150;  /* milliseconds */

  /* Select the power mode */
  /* Must be set before writing the sensor configuration */
  gas_sensor.power_mode = BME680_FORCED_MODE;

  /* Set the required sensor settings needed */
  set_required_settings = BME680_OST_SEL | BME680_OSP_SEL | BME680_OSH_SEL |
                          BME680_FILTER_SEL | BME680_GAS_SENSOR_SEL;

  /* Set the desired sensor configuration */
  rslt = bme680_set_sensor_settings(set_required_settings, &gas_sensor);

  /* Set the power mode */
  rslt = bme680_set_sensor_mode(&gas_sensor);

  if (rslt == BME680_OK) {
    ESP_LOGI(TAG, "BME680 sensor initialized");
  } else {
    ESP_LOGE(TAG, "BME680 initialization failed");
    return;
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

void user_delay_ms(uint32_t period) { vTaskDelay(period / portTICK_PERIOD_MS); }

#endif // HAS_BME