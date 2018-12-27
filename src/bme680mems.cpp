#ifdef HAS_BME

#include "bme680mems.h"

// Local logging tag
static const char TAG[] = "main";

bmeStatus_t bme_status;
TaskHandle_t BmeTask;

Bsec iaqSensor;

bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
};

// initialize BME680 sensor
int bme_init(void) {

  // block i2c bus access
  if (xSemaphoreTake(I2Caccess, (DISPLAYREFRESH_MS / portTICK_PERIOD_MS)) ==
      pdTRUE) {

    Wire.begin(HAS_BME);
    iaqSensor.begin(BME_ADDR, Wire);

    ESP_LOGI(TAG, "BSEC v%d.%d.%d.%d", iaqSensor.version.major,
             iaqSensor.version.minor, iaqSensor.version.major_bugfix,
             iaqSensor.version.minor_bugfix);

    iaqSensor.setConfig(bsec_config_iaq);

    if (checkIaqSensorStatus())
      ESP_LOGI(TAG, "BME680 sensor found and initialized");
    else {
      ESP_LOGE(TAG, "BME680 sensor not found");
      return 1;
    }

    iaqSensor.setTemperatureOffset((float)BME_TEMP_OFFSET);
    iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);

    if (checkIaqSensorStatus())
      ESP_LOGI(TAG, "BSEC subscription succesful");
    else {
      ESP_LOGE(TAG, "BSEC subscription error");
      return 1;
    }

    xSemaphoreGive(I2Caccess); // release i2c bus access

  } else {
    ESP_LOGE(TAG, "I2c bus busy - BME680 initialization error");
    return 1;
  }

} // bme_init()

// Helper function definitions
int checkIaqSensorStatus(void) {
  int rslt = 1; // true = 1 = no error, false = 0 = error

  if (iaqSensor.status != BSEC_OK) {
    rslt = 0;
    if (iaqSensor.status < BSEC_OK)
      ESP_LOGE(TAG, "BSEC error %d", iaqSensor.status);
    else
      ESP_LOGW(TAG, "BSEC warning %d", iaqSensor.status);
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    rslt = 0;
    if (iaqSensor.bme680Status < BME680_OK)
      ESP_LOGE(TAG, "BME680 error %d", iaqSensor.bme680Status);
    else
      ESP_LOGW(TAG, "BME680 warning %d", iaqSensor.bme680Status);
  }

  return rslt;
} // checkIaqSensorStatus()

// loop function which reads and processes data based on sensor settings
void bme_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

#ifdef HAS_BME
  while (checkIaqSensorStatus()) {

    // block i2c bus access
    if (xSemaphoreTake(I2Caccess, (DISPLAYREFRESH_MS / portTICK_PERIOD_MS)) ==
        pdTRUE) {

      if (iaqSensor.run()) { // If new data is available
        bme_status.raw_temperature = iaqSensor.rawTemperature;
        bme_status.raw_humidity = iaqSensor.rawHumidity;
        bme_status.temperature = iaqSensor.temperature;
        bme_status.humidity = iaqSensor.humidity;
        bme_status.pressure =
            (iaqSensor.pressure / 100.0); // conversion Pa -> hPa
        bme_status.iaq = iaqSensor.iaqEstimate;
        bme_status.iaq_accuracy = iaqSensor.iaqAccuracy;
        bme_status.gas = iaqSensor.gasResistance;
      }

      xSemaphoreGive(I2Caccess); // release i2c bus access
    }
  }
#endif
  ESP_LOGE(TAG, "BME task ended");
  vTaskDelete(BmeTask); // should never be reached

} // bme_loop()

#endif // HAS_BME