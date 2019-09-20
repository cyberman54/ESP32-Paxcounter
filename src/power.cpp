// Basic config
#include "globals.h"
#include "power.h"

// Local logging tag
static const char TAG[] = __FILE__;

#ifdef HAS_PMU

AXP20X_Class pmu;

void pover_event_IRQ(void) {
  // block i2c bus access
  if (I2C_MUTEX_LOCK()) {
    pmu.readIRQ();
    // put your power event handler code here

    if (pmu.isVbusOverVoltageIRQ())
      ESP_LOGI(TAG, "USB voltage %.1fV too high.", pmu.getVbusVoltage());
    if (pmu.isVbusPlugInIRQ())
      ESP_LOGI(TAG, "USB plugged, %.0fmAh @ %.1fV", pmu.getVbusCurrent(),
               pmu.getVbusVoltage());
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

    // wake up
    if (pmu.isPEKShortPressIRQ()) {
      ESP_LOGI(TAG, "Power Button short pressed.");
      AXP192_power(true);
    }
    // enter sleep mode
    if (pmu.isPEKLongtPressIRQ()) {
      ESP_LOGI(TAG, "Power Button long pressed.");
      AXP192_power(false);
      delay(20);
      esp_sleep_enable_ext1_wakeup(GPIO_SEL_38, ESP_EXT1_WAKEUP_ALL_LOW);
      esp_deep_sleep_start();
    }

    pmu.clearIRQ();
    I2C_MUTEX_UNLOCK(); // release i2c bus access
  } else
    ESP_LOGI(TAG, "Unknown PMU event.");
}

void AXP192_power(bool on) {

  if (on) {
    pmu.setPowerOutPut(AXP192_LDO2, AXP202_ON);
    pmu.setPowerOutPut(AXP192_LDO3, AXP202_ON);
    pmu.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
    pmu.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
    pmu.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
    pmu.setChgLEDMode(AXP20X_LED_LOW_LEVEL);
  } else {
    pmu.setPowerOutPut(AXP192_DCDC1, AXP202_OFF);
    pmu.setPowerOutPut(AXP192_EXTEN, AXP202_OFF);
    pmu.setPowerOutPut(AXP192_DCDC2, AXP202_OFF);
    pmu.setPowerOutPut(AXP192_LDO3, AXP202_OFF);
    pmu.setPowerOutPut(AXP192_LDO2, AXP202_OFF);
    pmu.setChgLEDMode(AXP20X_LED_OFF);
  }
}

void AXP192_displaypower(void) {
  if (pmu.isBatteryConnect())
    if (pmu.isChargeing())
      ESP_LOGI(TAG, "Battery charging %.0fmAh @ Temp %.1f°C",
               pmu.getBattChargeCurrent(), pmu.getTSTemp());
    else
      ESP_LOGI(TAG, "Battery not charging, Temp %.1f°C", pmu.getTSTemp());
  else
    ESP_LOGI(TAG, "No Battery");

  if (pmu.isVBUSPlug())
    ESP_LOGI(TAG, "USB present, %.0fmAh @ %.1fV", pmu.getVbusCurrent(),
             pmu.getVbusVoltage());
  else
    ESP_LOGI(TAG, "USB not present");
}

void AXP192_init(void) {

  // block i2c bus access
  if (I2C_MUTEX_LOCK()) {

    if (pmu.begin(Wire, AXP192_PRIMARY_ADDRESS))
      ESP_LOGI(TAG, "AXP192 PMU initialization failed");
    else {

      // switch power on
      pmu.setDCDC1Voltage(3300);
      pmu.adc1Enable(AXP202_BATT_CUR_ADC1, 1);
      AXP192_power(true);

      // I2C access of AXP202X library currently is not mutexable
      // so we better should disable AXP interrupts... ?
#ifdef PMU_INT
      pinMode(PMU_INT, INPUT_PULLUP);
      attachInterrupt(digitalPinToInterrupt(PMU_INT), PMUIRQ, FALLING);
      pmu.enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ |
                        AXP202_BATT_REMOVED_IRQ | AXP202_BATT_CONNECT_IRQ,
                    1);
      pmu.clearIRQ();
#endif // PMU_INT

      ESP_LOGI(TAG, "AXP192 PMU initialized, chip Temp %.1f°C", pmu.getTemp());
      AXP192_displaypower();
    }
    I2C_MUTEX_UNLOCK(); // release i2c bus access
  } else
    ESP_LOGE(TAG, "I2c bus busy - PMU initialization error");
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
  voltage = pmu.isVBUSPlug() ? 0xffff : pmu.getBattVoltage();
#else

#ifdef BAT_MEASURE_ADC
  // multisample ADC
  uint32_t adc_reading = 0;
  int adc_buf = 0;
  for (int i = 0; i < NO_OF_SAMPLES; i++) {
#ifndef BAT_MEASURE_ADC_UNIT // ADC1
    adc_reading += adc1_get_raw(adc_channel);
#else                        // ADC2
    ESP_ERROR_CHECK(adc2_get_raw(adc_channel, ADC_WIDTH_BIT_12, &adc_buf));
    adc_reading += adc_buf;
#endif
  }
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