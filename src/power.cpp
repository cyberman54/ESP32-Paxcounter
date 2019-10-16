// Basic config
#include "globals.h"
#include "power.h"

// Local logging tag
static const char TAG[] = __FILE__;

RTC_DATA_ATTR struct timeval sleep_enter_time;
RTC_DATA_ATTR runmode_t RTC_runmode = RUNMODE_NORMAL;

#ifdef HAS_PMU

AXP20X_Class pmu;

void AXP192_powerevent_IRQ(void) {

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

// esp32 sleep mode, can be exited by pressing user button
#ifdef HAS_BUTTON
  if (pmu.isPEKShortPressIRQ() && (RTC_runmode == RUNMODE_NORMAL)) {
    enter_deepsleep(0, HAS_BUTTON);
  }
#endif

  // shutdown power
  if (pmu.isPEKLongtPressIRQ()) {
    AXP192_power(false); // switch off Lora, GPS, display
    pmu.shutdown();      // switch off device
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
    // we don't cut off power of display, because then display blocks i2c bus
    // pmu.setPowerOutPut(AXP192_DCDC1, AXP202_OFF);
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

  if (pmu.begin(i2c_readBytes, i2c_writeBytes, AXP192_PRIMARY_ADDRESS) ==
      AXP_FAIL)
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
    if (!cnt)
      ret = 0xFF;
    uint16_t index = 0;
    while (Wire.available()) {
      if (index > len) {
        ret = 0xFF;
        goto finish;
      }
      data[index++] = Wire.read();
    }

  finish:
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
    // return ret ? 0xFF : ret;
    return ret ? ret : 0xFF;
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

void enter_deepsleep(const int wakeup_sec, const gpio_num_t wakeup_gpio) {

  if ((!wakeup_sec) && (!wakeup_gpio) && (RTC_runmode == RUNMODE_NORMAL))
    return;

  // set wakeup timer
  if (wakeup_sec)
    esp_sleep_enable_timer_wakeup(wakeup_sec * 1000000);

  // set wakeup gpio
  if (wakeup_gpio != NOT_A_PIN) {
    rtc_gpio_isolate(wakeup_gpio);
    esp_sleep_enable_ext1_wakeup(1ULL << wakeup_gpio, ESP_EXT1_WAKEUP_ALL_LOW);
  }

  // store LMIC counters and time
  RTCseqnoUp = LMIC.seqnoUp;
  RTCseqnoDn = LMIC.seqnoDn;

  // store sleep enter time
  gettimeofday(&sleep_enter_time, NULL);

  // halt interrupts accessing i2c bus
  mask_user_IRQ();

// switch off display
#ifdef HAS_DISPLAY
  shutdown_display();
#endif

// switch off wifi & ble
#if (BLECOUNTER)
  stop_BLEscan();
#endif

// switch off power if has PMU
#ifdef HAS_PMU
  AXP192_power(false); // switch off Lora, GPS, display
#endif

  // shutdown i2c bus
  i2c_deinit();

  // enter sleep mode
  esp_deep_sleep_start();
}

int exit_deepsleep(void) {

  struct timeval now;
  gettimeofday(&now, NULL);
  int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 +
                      (now.tv_usec - sleep_enter_time.tv_usec) / 1000;

  // switch on power if has PMU
#ifdef HAS_PMU
  AXP192_power(true); // power on Lora, GPS, display
#endif

  // re-init i2c bus
  void i2c_init();

  switch (esp_sleep_get_wakeup_cause()) {
  case ESP_SLEEP_WAKEUP_EXT1:
  case ESP_SLEEP_WAKEUP_TIMER:
    RTC_runmode = RUNMODE_WAKEUP;
    ESP_LOGI(TAG, "[%0.3f] wake up from deep sleep after %dms", sleep_time_ms);
    break;
  case ESP_SLEEP_WAKEUP_UNDEFINED:
  default:
    RTC_runmode = RUNMODE_NORMAL;
  }

  if (RTC_runmode == RUNMODE_WAKEUP)
    return sleep_time_ms;
  else
    return -1;
}

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
  voltage = pmu.isVBUSPlug() ? 0xffff : pmu.getBattVoltage();
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
#endif                       // BAT_MEASURE_ADC_UNIT
  adc_reading /= NO_OF_SAMPLES;
  // Convert ADC reading to voltage in mV
  voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_characs);
#endif                       // BAT_MEASURE_ADC

#ifdef BAT_VOLTAGE_DIVIDER
  voltage *= BAT_VOLTAGE_DIVIDER;
#endif // BAT_VOLTAGE_DIVIDER

#endif // HAS_PMU

  return voltage;
}