#if (HAS_BME)

#include "bmesensor.h"


bmeStatus_t bme_status = {0, 0, 0, 0, 0, 0, 0, 0};

Ticker bmecycler;

#define SEALEVELPRESSURE_HPA (1013.25)

#ifdef HAS_BME680
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

uint8_t bsecstate_buffer[BSEC_MAX_STATE_BLOB_SIZE] = {0};

Bsec iaqSensor;

#elif defined HAS_BME280

Adafruit_BME280 bme; // using I2C interface

// use these alternative constructors for other hw interface types
// Adafruit_BME280 bme(BME_CS); // hardware SPI
// Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

#elif defined HAS_BMP180

Adafruit_BMP085 bmp; // I2C

#endif

void setBMEIRQ() { xTaskNotify(irqHandlerTask, BME_IRQ, eSetBits); }

// initialize MEMS sensor
// return = 0 -> error / return = 1 -> success
int bme_init(void) {
  int rc = 0;

#ifdef HAS_BME680
  Wire.begin(HAS_BME680);
  iaqSensor.begin(BME680_ADDR, Wire);

  ESP_LOGI(TAG, "BSEC v%d.%d.%d.%d", iaqSensor.version.major,
           iaqSensor.version.minor, iaqSensor.version.major_bugfix,
           iaqSensor.version.minor_bugfix);

  iaqSensor.setConfig(bsec_config_iaq);
  loadState();
  iaqSensor.setTemperatureOffset((float)BME_TEMP_OFFSET);
  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);

  rc = checkIaqSensorStatus();

#elif defined HAS_BME280
  rc = bme.begin(BME280_ADDR);
#elif defined HAS_BMP180
  // Wire.begin(21, 22);
  rc = bmp.begin();

#endif

  if (rc)
    bmecycler.attach(BMECYCLE, setBMEIRQ); // start cyclic data transmit
  return rc;
} // bme_init()

#ifdef HAS_BME680

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
#endif

// store current BME sensor data in struct
void bme_storedata(bmeStatus_t *bme_store) {
  if (cfg.payloadmask & MEMS_DATA)

#ifdef HAS_BME680
    if (iaqSensor.run()) { // if new data is available
      bme_store->raw_temperature =
          iaqSensor.rawTemperature; // temperature in degree celsius
      bme_store->raw_humidity = iaqSensor.rawHumidity;
      bme_store->temperature = iaqSensor.temperature;
      bme_store->humidity =
          iaqSensor.humidity;           // humidity in % relative humidity x1000
      bme_store->pressure =             // pressure in Pascal
          (iaqSensor.pressure / 100.0); // conversion Pa -> hPa
      bme_store->iaq = iaqSensor.iaq;
      bme_store->iaq_accuracy = iaqSensor.iaqAccuracy;
      bme_store->gas = iaqSensor.gasResistance; // gas resistance in ohms
      updateState();
    }
#elif defined HAS_BME280
    bme_store->temperature = bme.readTemperature();
  bme_store->pressure = (bme.readPressure() / 100.0); // conversion Pa -> hPa
  // bme.readAltitude(SEALEVELPRESSURE_HPA);
  bme_store->humidity = bme.readHumidity();
  bme_store->iaq = 0; // IAQ feature not present with BME280
#elif defined HAS_BMP180
    bme_store->temperature = bmp.readTemperature();
  bme_store->pressure = (bmp.readPressure() / 100.0); // conversion Pa -> hPa
  // bme.readAltitude(SEALEVELPRESSURE_HPA);
  bme_store->iaq = 0; // IAQ feature not present with BME280
#endif
} // bme_storedata()

#ifdef HAS_BME680
void loadState(void) {
  if (cfg.bsecstate[BSEC_MAX_STATE_BLOB_SIZE] == BSEC_MAX_STATE_BLOB_SIZE) {
    // Existing state in NVS stored
    ESP_LOGI(TAG, "restoring BSEC state from NVRAM");
    memcpy(bsecstate_buffer, cfg.bsecstate, BSEC_MAX_STATE_BLOB_SIZE);
    iaqSensor.setState(bsecstate_buffer);
    checkIaqSensorStatus();
  } else // no state stored
    ESP_LOGI(TAG,
             "no BSEC state stored in NVRAM, starting sensor with defaults");
}

void updateState(void) {
  bool update = false;
  static uint16_t stateUpdateCounter = 0;

  if (stateUpdateCounter == 0) {
    // first state update when IAQ accuracy is >= 1
    if (iaqSensor.iaqAccuracy >= 1) {
      update = true;
      stateUpdateCounter++;
    }
  } else {
    /* Update every STATE_SAVE_PERIOD minutes */
    if ((long)(millis() - stateUpdateCounter * STATE_SAVE_PERIOD) >= 0) {
      update = true;
      stateUpdateCounter++;
    }
  }

  if (update) {
    iaqSensor.getState(bsecstate_buffer);
    checkIaqSensorStatus();
    memcpy(cfg.bsecstate, bsecstate_buffer, BSEC_MAX_STATE_BLOB_SIZE);
    cfg.bsecstate[BSEC_MAX_STATE_BLOB_SIZE] = BSEC_MAX_STATE_BLOB_SIZE;
    ESP_LOGI(TAG, "saving BSEC state to NVRAM");
    saveConfig(false);
  }
}
#endif

#endif // HAS_BME