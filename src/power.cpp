// Basic config
#include "globals.h"
#include "power.h"

// Local logging tag
static const char TAG[] = __FILE__;

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

// short press -> esp32 deep sleep mode, can be exited by pressing user button
#ifdef HAS_BUTTON
  if (pmu.isPEKShortPressIRQ() && (RTC_runmode == RUNMODE_NORMAL)) {
    enter_deepsleep(0, HAS_BUTTON);
  }
#endif

  // long press -> shutdown power, can be exited by another longpress
  if (pmu.isPEKLongtPressIRQ()) {
    AXP192_power(pmu_power_off); // switch off Lora, GPS, display
    pmu.shutdown();              // switch off device
  }

  pmu.clearIRQ();

  // refresh stored voltage value
  read_battlevel();
}

void AXP192_power(pmu_power_t powerlevel) {

  switch (powerlevel) {

  case pmu_power_off:
    pmu.setChgLEDMode(AXP20X_LED_OFF);
    pmu.setPowerOutPut(AXP192_DCDC1, AXP202_OFF);
    pmu.setPowerOutPut(AXP192_LDO3, AXP202_OFF);
    pmu.setPowerOutPut(AXP192_LDO2, AXP202_OFF);
    // pmu.setPowerOutPut(AXP192_DCDC3, AXP202_OFF);
    break;

  case pmu_power_sleep:
    pmu.setChgLEDMode(AXP20X_LED_BLINK_1HZ);
    // we don't cut off DCDC1, because then display blocks i2c bus
    pmu.setPowerOutPut(AXP192_LDO3, AXP202_OFF); // gps off
    pmu.setPowerOutPut(AXP192_LDO2, AXP202_OFF); // lora off
    break;

  default:                                       // all rails power on
    pmu.setPowerOutPut(AXP192_LDO2, AXP202_ON);  // Lora on T-Beam V1.0
    pmu.setPowerOutPut(AXP192_LDO3, AXP202_ON);  // Gps on T-Beam V1.0
    pmu.setPowerOutPut(AXP192_DCDC1, AXP202_ON); // OLED on T-Beam v1.0
    pmu.setChgLEDMode(AXP20X_LED_LOW_LEVEL);
    break;
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
    AXP192_power(pmu_power_on);

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

#endif // HAS_PMU

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

uint16_t read_voltage(void) {
  uint16_t voltage = 0;

#ifdef HAS_PMU
  voltage = pmu.getBattVoltage();
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

uint8_t read_battlevel(mapFn_t mapFunction) {
  // returns the estimated battery level in values 0 ... 100 [percent]
#ifdef HAS_IP5306
  return IP5306_GetBatteryLevel();
#else
  const uint16_t batt_voltage = read_voltage();
  if (batt_voltage <= BAT_MIN_VOLTAGE)
    return 0;
  else if (batt_voltage >= BAT_MAX_VOLTAGE)
    return 100;
  else
    return (*mapFunction)(batt_voltage, BAT_MIN_VOLTAGE, BAT_MAX_VOLTAGE);
#endif
}

bool batt_sufficient() {
#if (defined HAS_PMU || defined BAT_MEASURE_ADC || defined HAS_IP5306)
  return (batt_level > OTA_MIN_BATT);
#else
  return true; // we don't know batt level
#endif
}

#ifdef HAS_IP5306

// IP5306 code snippet was taken from
// https://gist.github.com/me-no-dev/7702f08dd578de5efa47caf322250b57

#define IP5306_REG_SYS_0 0x00
#define IP5306_REG_SYS_1 0x01
#define IP5306_REG_SYS_2 0x02
#define IP5306_REG_CHG_0 0x20
#define IP5306_REG_CHG_1 0x21
#define IP5306_REG_CHG_2 0x22
#define IP5306_REG_CHG_3 0x23
#define IP5306_REG_CHG_4 0x24
#define IP5306_REG_READ_0 0x70
#define IP5306_REG_READ_1 0x71
#define IP5306_REG_READ_2 0x72
#define IP5306_REG_READ_3 0x77
#define IP5306_REG_READ_4 0x78

#define IP5306_LEDS2PCT(byte)                                                  \
  ((byte & 0x01 ? 25 : 0) + (byte & 0x02 ? 25 : 0) + (byte & 0x04 ? 25 : 0) +  \
   (byte & 0x08 ? 25 : 0))

uint8_t ip5306_get_bits(uint8_t reg, uint8_t index, uint8_t bits) {
  uint8_t value;
  if (i2c_readBytes(IP5306_PRIMARY_ADDRESS, reg, &value, 1) == 0xff) {
    ESP_LOGW(TAG, "IP5306 get bits fail: 0x%02x", reg);
    return 0;
  }
  return (value >> index) & ((1 << bits) - 1);
}

void ip5306_set_bits(uint8_t reg, uint8_t index, uint8_t bits, uint8_t value) {
  uint8_t mask = (1 << bits) - 1, v;
  if (i2c_readBytes(IP5306_PRIMARY_ADDRESS, reg, &v, 1) == 0xff) {
    ESP_LOGW(TAG, "IP5306 register read fail: 0x%02x", reg);
    return;
  }
  v &= ~(mask << index);
  v |= ((value & mask) << index);

  if (i2c_writeBytes(IP5306_PRIMARY_ADDRESS, reg, &v, 1) == 0xff)
    ESP_LOGW(TAG, "IP5306 register write fail: 0x%02x", reg);
}

uint8_t IP5306_GetPowerSource(void) {
  return ip5306_get_bits(IP5306_REG_READ_0, 3, 1); // 0:BAT, 1:VIN
}

uint8_t IP5306_GetBatteryFull(void) {
  return ip5306_get_bits(IP5306_REG_READ_1, 3, 1); // 0:CHG/DIS, 1:FULL
}

uint8_t IP5306_GetBatteryLevel(void) {
  uint8_t state = (~ip5306_get_bits(IP5306_REG_READ_4, 4, 4)) & 0x0F;
  // LED[0-4] State (inverted)
  return IP5306_LEDS2PCT(state);
}

void printIP5306Stats(void) {
  bool usb = IP5306_GetPowerSource();
  bool full = IP5306_GetBatteryFull();
  uint8_t level = IP5306_GetBatteryLevel();
  ESP_LOGI(TAG,
           "IP5306: Power Source: %s, Battery State: %s, Battery Level: %u%%",
           usb ? "USB" : "BATTERY",
           full ? "CHARGED" : (usb ? "CHARGING" : "DISCHARGING"), level);
}

#endif // HAS_IP5306