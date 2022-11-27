// Basic config
#include "globals.h"
#include "power.h"


int8_t batt_level = -1; // percent batt level, global variable, -1 means no batt

#ifdef BAT_MEASURE_ADC
esp_adc_cal_characteristics_t *adc_characs =
    (esp_adc_cal_characteristics_t *)calloc(
        1, sizeof(esp_adc_cal_characteristics_t));

#ifndef BAT_MEASURE_ADC_UNIT // ADC1
static const adc1_channel_t adc_channel = BAT_MEASURE_ADC;
#else // ADC2
static const adc2_channel_t adc_channel = BAT_MEASURE_ADC;
RTC_NOINIT_ATTR uint64_t RTC_reg_b;
#endif
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;
#endif // BAT_MEASURE_ADC

#ifdef HAS_PMU
XPowersPMU pmu;

void IRAM_ATTR PMUIRQ() { doIRQ(PMU_IRQ); }

void AXP192_powerevent_IRQ(void) {
  pmu.getIrqStatus();

  if (pmu.isVbusOverVoltageIrq())
    ESP_LOGI(TAG, "USB voltage %.2fV too high.", pmu.getVbusVoltage() / 1000);
  if (pmu.isVbusInsertIrq())
    ESP_LOGI(TAG, "USB plugged, %.2fV @ %.0mA", pmu.getVbusVoltage() / 1000,
             pmu.getVbusCurrent());
  if (pmu.isVbusRemoveIrq())
    ESP_LOGI(TAG, "USB unplugged.");
  if (pmu.isBatInsertIrq())
    ESP_LOGI(TAG, "Battery is connected.");
  if (pmu.isBatRemoveIrq())
    ESP_LOGI(TAG, "Battery was removed.");
  if (pmu.isBatChagerStartIrq())
    ESP_LOGI(TAG, "Battery charging started.");
  if (pmu.isBatChagerDoneIrq())
    ESP_LOGI(TAG, "Battery charging done.");
  if (pmu.isBattTempLowIrq())
    ESP_LOGI(TAG, "Battery high temperature.");
  if (pmu.isBattTempHighIrq())
    ESP_LOGI(TAG, "Battery low temperature.");

  // PEK button handling:
  // long press -> shutdown power, must be exited by another longpress
  if (pmu.isPekeyLongPressIrq())
    AXP192_power(pmu_power_off); // switch off Lora, GPS, display
#ifdef HAS_BUTTON
  // short press -> esp32 deep sleep mode, must be exited by user button
  if (pmu.isPekeyShortPressIrq())
    enter_deepsleep(0UL, HAS_BUTTON);
#endif

  pmu.clearIrqStatus();

  // refresh stored voltage value
  read_battlevel();
}

void AXP192_power(pmu_power_t powerlevel) {
  switch (powerlevel) {
  case pmu_power_off:
    pmu.setChargingLedMode(XPOWERS_CHG_LED_OFF);
    pmu.shutdown();
    break;
  case pmu_power_sleep:
    pmu.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);
    // we don't cut off DCDC1, because OLED display will then block i2c bus
    // pmu.disableDC1(); // OLED off
    pmu.disableLDO3(); // gps off
    pmu.disableLDO2(); // lora off
    pmu.enableSleep();
    break;
  case pmu_power_on:
  default:
    pmu.enableLDO2(); // Lora on T-Beam V1.0/1.1
    pmu.enableLDO3(); // Gps on T-Beam V1.0/1.1
    pmu.enableDC1();  // OLED on T-Beam v1.0/1.1
    pmu.setChargingLedMode(XPOWERS_CHG_LED_ON);
    break;
  }
}

void AXP192_showstatus(void) {
  if (pmu.isBatteryConnect())
    if (pmu.isCharging())
      ESP_LOGI(TAG, "Battery charging, %.2fV @ %.0fmAh",
               pmu.getBattVoltage() / 1000.0, pmu.getBatteryChargeCurrent());
    else
      ESP_LOGI(TAG, "Battery not charging");
  else
    ESP_LOGI(TAG, "Battery not present");

  if (pmu.isVbusIn())
    ESP_LOGI(TAG, "USB powered, %.0fmW",
             pmu.getVbusVoltage() / 1000 * pmu.getVbusCurrent());
  else
    ESP_LOGI(TAG, "USB not present");
}

void AXP192_init(void) {
  if (!pmu.begin(Wire, AXP192_PRIMARY_ADDRESS, SCL, SDA))
    ESP_LOGI(TAG, "AXP192 PMU initialization failed");
  else {
    ESP_LOGD(TAG, "AXP192 ChipID:0x%x", pmu.getChipID());

    // set pmu operating voltages
    pmu.setSysPowerDownVoltage(2700);
    pmu.setVbusVoltageLimit(XPOWERS_AXP192_VBUS_VOL_LIM_4V5);
    pmu.setVbusCurrentLimit(XPOWERS_AXP192_VBUS_CUR_LIM_OFF);

    // set device operating voltages
    pmu.setDC1Voltage(3300);  // for external OLED display
    pmu.setLDO2Voltage(3300); // LORA VDD 3v3
    pmu.setLDO3Voltage(3300); // GPS VDD 3v3

    // configure PEK button settings
    pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    pmu.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);

    // set battery temperature sensing pin off to save power
    pmu.disableTSPinMeasure();

    // Enable internal ADC detection
    pmu.enableBattDetection();
    pmu.enableVbusVoltageMeasure();
    pmu.enableBattVoltageMeasure();
    pmu.enableSystemVoltageMeasure();

#ifdef PMU_INT
    pinMode(PMU_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PMU_INT), PMUIRQ, FALLING);
    // disable all interrupts
    pmu.disableIRQ(XPOWERS_AXP192_ALL_IRQ);
    // clear all interrupt flags
    pmu.clearIrqStatus();
    // enable the required interrupt function
    pmu.enableIRQ(XPOWERS_AXP192_BAT_INSERT_IRQ |
                  XPOWERS_AXP192_BAT_REMOVE_IRQ | // BATTERY
                  XPOWERS_AXP192_VBUS_INSERT_IRQ |
                  XPOWERS_AXP192_VBUS_REMOVE_IRQ | // VBUS
                  XPOWERS_AXP192_PKEY_SHORT_IRQ |
                  XPOWERS_AXP192_PKEY_LONG_IRQ | // POWER KEY
                  XPOWERS_AXP192_BAT_CHG_DONE_IRQ |
                  XPOWERS_AXP192_BAT_CHG_START_IRQ // CHARGE
    );
#endif // PMU_INT

// set charging parameters according to user settings if we have (see power.h)
#ifdef PMU_CHG_CURRENT
    pmu.setChargerConstantCurr(PMU_CHG_CURRENT);
    pmu.setChargeTargetVoltage(PMU_CHG_CUTOFF);
    pmu.enableCharge();
#endif

    // switch power rails on
    AXP192_power(pmu_power_on);
    ESP_LOGI(TAG, "AXP192 PMU initialized");
  }
}

#endif // HAS_PMU

void calibrate_voltage(void) {
#ifdef BAT_MEASURE_ADC
// configure ADC
#ifndef BAT_MEASURE_ADC_UNIT // ADC1
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(adc_channel, atten);
#else // ADC2
  adc2_config_channel_atten(adc_channel, atten);
  // ADC2 wifi bug workaround, see
  // https://github.com/espressif/arduino-esp32/issues/102
  RTC_reg_b = READ_PERI_REG(SENS_SAR_READ_CTRL2_REG);
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
    // ADC2 wifi bug workaround, see
    // https://github.com/espressif/arduino-esp32/issues/102
    WRITE_PERI_REG(SENS_SAR_READ_CTRL2_REG, RTC_reg_b);
    SET_PERI_REG_MASK(SENS_SAR_READ_CTRL2_REG, SENS_SAR2_DATA_INV);
    adc2_get_raw(adc_channel, ADC_WIDTH_BIT_12, &adc_buf);
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

int8_t read_battlevel(mapFn_t mapFunction) {
  // returns the estimated battery level in values 0 ... 100 [percent]
  uint8_t batt_percent = 0;
#ifdef HAS_IP5306
  batt_percent = IP5306_GetBatteryLevel();
#elif defined HAS_PMU
  batt_percent = pmu.getBatteryPercent();
#else
  const uint16_t batt_voltage = read_voltage();
  if (batt_voltage <= BAT_MIN_VOLTAGE)
    batt_percent = 0;
  else if (batt_voltage >= BAT_MAX_VOLTAGE)
    batt_percent = 100;
  else
    batt_percent =
        (*mapFunction)(batt_voltage, BAT_MIN_VOLTAGE, BAT_MAX_VOLTAGE);
#endif

#if (HAS_LORA)
  // set the battery status value to send by LMIC in MAC Command
  // DevStatusAns. Available defines in lorabase.h:
  //   MCMD_DEVS_EXT_POWER   = 0x00, // external power supply
  //   MCMD_DEVS_BATT_MIN    = 0x01, // min battery value
  //   MCMD_DEVS_BATT_MAX    = 0xFE, // max battery value
  //   MCMD_DEVS_BATT_NOINFO = 0xFF, // unknown battery level
  // we calculate the applicable value from MCMD_DEVS_BATT_MIN to
  // MCMD_DEVS_BATT_MAX from batt_percent value

  if (batt_percent == -1)
    LMIC_setBatteryLevel(MCMD_DEVS_BATT_NOINFO);
  else
    LMIC_setBatteryLevel(batt_percent / 100.0 *
                         (MCMD_DEVS_BATT_MAX - MCMD_DEVS_BATT_MIN + 1));

// overwrite calculated value if we have external power
#ifdef HAS_PMU
  if (pmu.isVbusIn())
    LMIC_setBatteryLevel(MCMD_DEVS_EXT_POWER);
#elif defined HAS_IP5306
  if (IP5306_GetPowerSource())
    LMIC_setBatteryLevel(MCMD_DEVS_EXT_POWER);
#endif // HAS_PMU

#endif // HAS_LORA

  return batt_percent;
}

bool batt_sufficient() {
#if (defined HAS_PMU || defined BAT_MEASURE_ADC || defined HAS_IP5306)
  if (batt_level > 0) // we have a battery percent value
    return (batt_level > OTA_MIN_BATT);
  else
#endif
    return true; // we don't know batt level
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

void IP5306_SetChargerEnabled(uint8_t v) {
  ip5306_set_bits(IP5306_REG_SYS_0, 4, 1, v); // 0:dis,*1:en
}

void IP5306_SetChargeCutoffVoltage(uint8_t v) {
  ip5306_set_bits(IP5306_REG_CHG_2, 2, 2,
                  v); //*0:4.2V, 1:4.3V, 2:4.35V, 3:4.4V
}

void IP5306_SetEndChargeCurrentDetection(uint8_t v) {
  ip5306_set_bits(IP5306_REG_CHG_1, 6, 2,
                  v); // 0:200mA, 1:400mA, *2:500mA, 3:600mA
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

void IP5306_init(void) {
  IP5306_SetChargerEnabled(1);
  IP5306_SetChargeCutoffVoltage(PMU_CHG_CUTOFF);
  IP5306_SetEndChargeCurrentDetection(PMU_CHG_CURRENT);
}

#endif // HAS_IP5306