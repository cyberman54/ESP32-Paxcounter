#ifdef HAS_BME

#include "bme680read.h"

// Local logging tag
static const char TAG[] = "main";

#define SEALEVELPRESSURE_HPA (1013.25)

// I2C Bus interface
Adafruit_BME680 bme;

bmeStatus_t bme_status;

void bme_init(void) {
  // initialize BME680 sensor using default i2c address 0x77
  if (bme.begin(HAS_BME)) {
    // Set up oversampling and filter initialization
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320*C for 150 ms
    ESP_LOGI(TAG, "BME680 chip found and initialized");
  } else
    ESP_LOGE(TAG, "BME680 chip not found on i2c bus");
}

bool bme_read(void) {
  bool ret = bme.performReading();
  if (ret) {
    // read current BME data and buffer in global struct
    bme_status.temperature = bme.temperature;
    bme_status.pressure = (uint16_t)(bme.pressure / 100.0); // convert Pa -> hPa
    bme_status.humidity = bme.humidity;
    bme_status.gas_resistance = (uint16_t)(bme.gas_resistance / 1000.0); // convert Ohm -> kOhm
    bme_status.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    ESP_LOGI(TAG, "BME680 sensor data read success");
  } else {
    ESP_LOGI(TAG, "BME680 sensor read error");
  }
  return ret;
}

#endif // HAS_BME