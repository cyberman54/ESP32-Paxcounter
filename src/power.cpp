// Basic config
#include "globals.h"
#include "power.h"

// Local logging tag
static const char TAG[] = __FILE__;

#ifdef HAS_PMU

AXP20X_Class pmu;

void power_event_IRQ(void) {

  pmu.readIRQ();

  if (pmu.isVbusOverVoltageIRQ())
    ESP_LOGI(TAG, "USB voltage %.2fV too high.", pmu.getVbusVoltage() / 1000);
  if (pmu.isVbusPlugInIRQ())
    ESP_LOGI(TAG, "USB plugged, %.2fV @ %.0mA", pmu.getVbusVoltage() / 1000,
             pmu.getVbusCurrent());
  if (pmu.isVbusRemoveIRQ())
    ESP_LOGI(TAG, "USB unplugged.");

  if (pmu.isBattPlugInIRQ())
    ESP_LOGI(TAG, "Battery is connected.");
  if (pmu.isBattRemoveIRQ())
    ESP_LOGI(TAG, "Battery was removed.");
  if (pmu.isChargingIRQ())
    ESP_LOGI(TAG, "Battery charging.");
  if (pmu.isChargingDoneIRQ())
    ESP_LOGI(TAG, "Battery charging done.");
  if (pmu.isBattTempLowIRQ())
    ESP_LOGI(TAG, "Battery high temperature.");
  if (pmu.isBattTempHighIRQ())
    ESP_LOGI(TAG, "Battery low temperature.");

  // display on/off
  if (pmu.isPEKShortPressIRQ()) {
    cfg.screenon = !cfg.screenon;
  }

  // shutdown power
  if (pmu.isPEKLongtPressIRQ()) {
    AXP192_power(false); // switch off Lora, GPS, display
    pmu.shutdown();
  }

  pmu.clearIRQ();

  // refresh stored voltage value
  read_voltage();
}

void AXP192_power(bool on) {
  if (on) {
    pmu.setPowerOutPut(AXP192_LDO2, AXP202_ON);  // Lora on T-Beam V1.0
    pmu.setPowerOutPut(AXP192_LDO3, AXP202_ON);  // Gps on T-Beam V1.0
    pmu.setPowerOutPut(AXP192_DCDC1, AXP202_ON); // OLED on T-Beam v1.0
    // pmu.setChgLEDMode(AXP20X_LED_LOW_LEVEL);
    pmu.setChgLEDMode(AXP20X_LED_BLINK_1HZ);
  } else {
    pmu.setChgLEDMode(AXP20X_LED_OFF);
    pmu.setPowerOutPut(AXP192_DCDC1, AXP202_OFF);
    pmu.setPowerOutPut(AXP192_LDO3, AXP202_OFF);
    pmu.setPowerOutPut(AXP192_LDO2, AXP202_OFF);
  }
}

void AXP192_showstatus(void) {

  if (pmu.isBatteryConnect())
    if (pmu.isChargeing())
      ESP_LOGI(TAG, "Battery charging, %.2fV @ %.0fmAh",
               pmu.getBattVoltage() / 1000, pmu.getBattChargeCurrent());
    else
      ESP_LOGI(TAG, "Battery not charging");
  else
    ESP_LOGI(TAG, "No Battery");

  if (pmu.isVBUSPlug())
    ESP_LOGI(TAG, "USB powered, %.0fmW",
             pmu.getVbusVoltage() / 1000 * pmu.getVbusCurrent());
  else
    ESP_LOGI(TAG, "USB not present");
}

void AXP192_init(void) {

  if (pmu.begin(i2c_readBytes, i2c_writeBytes, AXP192_PRIMARY_ADDRESS) == AXP_FAIL)
    ESP_LOGI(TAG, "AXP192 PMU initialization failed");
  else {

    // configure AXP192
    pmu.setDCDC1Voltage(3300);              // for external OLED display
    pmu.setTimeOutShutdown(false);          // no automatic shutdown
    pmu.setTSmode(AXP_TS_PIN_MODE_DISABLE); // TS pin mode off to save power

    // switch ADCs on
    pmu.adc1Enable(AXP202_BATT_VOL_ADC1, true);
    pmu.adc1Enable(AXP202_BATT_CUR_ADC1, true);
    pmu.adc1Enable(AXP202_VBUS_VOL_ADC1, true);
    pmu.adc1Enable(AXP202_VBUS_CUR_ADC1, true);

    // switch power rails on
    AXP192_power(true);

#ifdef PMU_INT
    pinMode(PMU_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PMU_INT), PMUIRQ, FALLING);
    pmu.enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ |
                      AXP202_BATT_REMOVED_IRQ | AXP202_BATT_CONNECT_IRQ |
                      AXP202_CHARGING_FINISHED_IRQ,
                  1);
    pmu.clearIRQ();
#endif // PMU_INT

    ESP_LOGI(TAG, "AXP192 PMU initialized");
  }
}

// helper functions for mutexing i2c access
uint8_t i2c_readBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
  if (I2C_MUTEX_LOCK()) {

    uint8_t ret = 0;
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    uint8_t cnt = Wire.requestFrom(addr, (uint8_t)len, (uint8_t)1);
    if (!cnt) {
      ret = 0xFF;
    }
    uint16_t index = 0;
    while (Wire.available()) {
      if (index > len)
        return 0xFF;
      data[index++] = Wire.read();
    }

    I2C_MUTEX_UNLOCK(); // release i2c bus access
    return ret;
  } else {
    ESP_LOGW(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
    return 0xFF;
  }
}

uint8_t i2c_writeBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
  if (I2C_MUTEX_LOCK()) {
    
    uint8_t ret = 0;
    Wire.beginTransmission(addr);
    Wire.write(reg);
    for (uint16_t i = 0; i < len; i++) {
      Wire.write(data[i]);
    }
    ret = Wire.endTransmission();

    I2C_MUTEX_UNLOCK(); // release i2c bus access
    return ret ? 0xFF : ret;
    //return ret ? ret : 0xFF;
  } else {
    ESP_LOGW(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
    return 0xFF;
  }
}

#endif // HAS_PMU

#ifdef BAT_MEASURE_ADC
esp_adc_cal_characteristics_t *adc_characs =
    (esp_adc_cal_characteristics_t *)calloc(
        1, sizeof(esp_adc_cal_characteristics_t));

#ifndef BAT_MEASURE_ADC_UNIT // ADC1
static const adc1_channel_t adc_channel = BAT_MEASURE_ADC;
#else // ADC2
static const adc2_channel_t adc_channel = BAT_MEASURE_ADC;
#endif
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

#endif // BAT_MEASURE_ADC

void calibrate_voltage(void) {
#ifdef BAT_MEASURE_ADC
// configure ADC
#ifndef BAT_MEASURE_ADC_UNIT // ADC1
  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
  ESP_ERROR_CHECK(adc1_config_channel_atten(adc_channel, atten));
#else // ADC2
      // ESP_ERROR_CHECK(adc2_config_width(ADC_WIDTH_BIT_12));
  ESP_ERROR_CHECK(adc2_config_channel_atten(adc_channel, atten));
#endif
  // calibrate ADC
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
      unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_characs);
  // show ADC characterization base
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    ESP_LOGI(TAG,
             "ADC characterization based on Two Point values stored in eFuse");
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    ESP_LOGI(TAG,
             "ADC characterization based on reference voltage stored in eFuse");
  } else {
    ESP_LOGI(TAG, "ADC characterization based on default reference voltage");
  }
#endif
}

bool batt_sufficient() {
#if (defined HAS_PMU || defined BAT_MEASURE_ADC)
  uint16_t volts = read_voltage();
  return ((volts < 1000) ||
          (volts > OTA_MIN_BATT)); // no battery or battery sufficient
#else
  return true;
#endif
}

uint16_t read_voltage() {
  uint16_t voltage = 0;

#ifdef HAS_PMU
  //   if (!I2C_MUTEX_LOCK())
  //     ESP_LOGW(TAG, "[%0.3f] i2c mutex lock failed", millis() / 1000.0);
  //   else {
  voltage = pmu.isVBUSPlug() ? 0xffff : pmu.getBattVoltage();
  //     I2C_MUTEX_UNLOCK();
  //   }
#else

#ifdef BAT_MEASURE_ADC
  // multisample ADC
  uint32_t adc_reading = 0;
#ifndef BAT_MEASURE_ADC_UNIT // ADC1
  for (int i = 0; i < NO_OF_SAMPLES; i++) {
    adc_reading += adc1_get_raw(adc_channel);
  }
#else                        // ADC2
  int adc_buf = 0;
  for (int i = 0; i < NO_OF_SAMPLES; i++) {
    ESP_ERROR_CHECK(adc2_get_raw(adc_channel, ADC_WIDTH_BIT_12, &adc_buf));
    adc_reading += adc_buf;
  }
#endif
  adc_reading /= NO_OF_SAMPLES;
  // Convert ADC reading to voltage in mV
  voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_characs);
#endif // BAT_MEASURE_ADC

#ifdef BAT_VOLTAGE_DIVIDER
  voltage *= BAT_VOLTAGE_DIVIDER;
#endif // BAT_VOLTAGE_DIVIDER

#endif // HAS_PMU

  return voltage;
}