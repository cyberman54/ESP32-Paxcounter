#ifdef HAS_BME280

#include "bme280.h"

// Local logging tag
static const char TAG[] = __FILE__;

bmeStatus_t bme_status;
TaskHandle_t Bme280Task;

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

// initialize BME280 sensor
int bme280_init(void) {

  bool status;
  // return = 0 -> error / return = 1 -> success

  // block i2c bus access
  if (I2C_MUTEX_LOCK()) {
    status = bme.begin(BME280_ADDR);  
    if (!status) {
        ESP_LOGE(TAG, "BME280 sensor not found");
        goto error;
    }
    ESP_LOGI(TAG, "BME280 sensor found and initialized");
  } else {
    ESP_LOGE(TAG, "I2c bus busy - BME280 initialization error");
    goto error;
  }

  I2C_MUTEX_UNLOCK(); // release i2c bus access
  return 1;

error:
  I2C_MUTEX_UNLOCK(); // release i2c bus access
  return 0;

} // bme_init()

// loop function which reads and processes data based on sensor settings
void bme280_loop(void *pvParameters) {
#ifdef HAS_BME280
  while (1) {
    if (I2C_MUTEX_LOCK()) {
      bme_status.temperature = bme.readTemperature();
      bme_status.pressure = (bme.readPressure() / 100.0); // conversion Pa -> hPa
      // bme.readAltitude(SEALEVELPRESSURE_HPA);
      bme_status.humidity = bme.readHumidity();
      I2C_MUTEX_UNLOCK();
    }
  }
#endif
  ESP_LOGE(TAG, "BME task ended");
  vTaskDelete(Bme280Task); // should never be reached

} // bme_loop()

#endif // HAS_BME280