#ifdef HAS_BME

#include "globals.h"

// Local logging tag
static const char TAG[] = "main";

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#define SEALEVELPRESSURE_HPA (1013.25)

// I2C Bus interface
Adafruit_BME680 bme;

bmeStatus_t bme_status;
TaskHandle_t BmeTask;

// BME680 read loop Task
void bme_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  // initialize BME680 sensor
  if (!bme.begin()) {
    ESP_LOGE(TAG, "BME680 chip not found on i2c bus, bus error %d. "
                  "Stopping BME task.");
    vTaskDelete(BmeTask);
  } else {
    ESP_LOGI(TAG, "BME680 chip found.");
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  // read loop
  while (1) {

    vTaskDelay(10000 / portTICK_PERIOD_MS);

    if (!bme.performReading()) {
      ESP_LOGE(TAG, "BME680 read error");
      continue;
    } else {
      // read current BME data and buffer in global struct
      bme_status.temperature = bme.temperature;
      bme_status.pressure = bme.pressure / 100.0;
      bme_status.humidity = bme.humidity;
      bme_status.gas_resistance = bme.gas_resistance / 1000.0;
      bme_status.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    }
  } // end of infinite loop

  vTaskDelete(NULL); // shoud never be reached

} // bme_loop()

#endif // HAS_BME