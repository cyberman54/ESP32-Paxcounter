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

static const uint8_t i2c_sda = PMU_SDA;
static const uint8_t i2c_scl = PMU_SCL;

XPowersLibInterface *pmu = NULL;

bool AXPxxx_init() {
#if defined(XPOWERS_CHIP_AXP2101)
    if (!pmu) {
        pmu = new XPowersAXP2101(Wire1, i2c_sda, i2c_scl, AXP192_PRIMARY_ADDRESS);
        if (!pmu->init()) {
            delete pmu;
            pmu = NULL;
        } else {
            ESP_LOGI(TAG, "AXP2101 PMU init succeeded, using AXP2101 PMU\n");
            AXP2101_init();
        }
    }
#endif
#if defined(XPOWERS_CHIP_AXP192)
    if (!pmu) {
        pmu = new XPowersAXP192(Wire1, i2c_sda, i2c_scl, AXP192_PRIMARY_ADDRESS);
        if (!pmu->init()) {
            delete pmu;
            pmu = NULL;
        } else {
            ESP_LOGI(TAG, "AXP192 PMU init succeeded, using AXP192 PMU\n");
            AXP192_init();
        }
    }
#endif
#if defined(XPOWERS_CHIP_AXP202)
    if (!pmu) {
        pmu = new XPowersAXP202(Wire1, i2c_sda, i2c_scl, AXP192_PRIMARY_ADDRESS);
        if (!pmu->init()) {
            delete pmu;
            pmu = NULL;
        } else {
            ESP_LOGI(TAG, "AXP202 PMU init succeeded, using AXP202 PMU\n");
            AXP202_init();
        }
    }
#endif

    if (!pmu) {
        ESP_LOGI(TAG, "PMU not detected, please check..");
    }
    return pmu != nullptr;
}


void AXPxxx_powerevent_IRQ(void)
{
  if (pmu != nullptr) {
    switch(pmu->getChipModel()) {
#if defined(XPOWERS_CHIP_AXP192)
      case XPOWERS_AXP192: 
        AXP192_powerevent_IRQ();
        break;
#endif
#if defined(XPOWERS_CHIP_AXP2101)
      case XPOWERS_AXP2101: 
        AXP2101_powerevent_IRQ();
        break;
#endif
#if defined(XPOWERS_CHIP_AXP202)
      case XPOWERS_AXP202:
        AXP202_powerevent_IRQ();
        break;
#endif
    }
  }
}


void AXPxxx_power(pmu_power_t powerlevel)
{
  if (pmu != nullptr) {
    switch(pmu->getChipModel()) {
#if defined(XPOWERS_CHIP_AXP192)
      case XPOWERS_AXP192: 
        AXP192_power(powerlevel);
        break;
#endif
#if defined(XPOWERS_CHIP_AXP2101)
      case XPOWERS_AXP2101: 
        AXP2101_power(powerlevel);
        break;
#endif
#if defined(XPOWERS_CHIP_AXP202)
      case XPOWERS_AXP202:
        AXP202_power(powerlevel);
        break;
#endif
    }
  }
}

void AXPxxx_showstatus(void)
{
  if (pmu != nullptr) {
    switch(pmu->getChipModel()) {
#if defined(XPOWERS_CHIP_AXP192)
      case XPOWERS_AXP192: 
        AXP192_showstatus();
        break;
#endif
#if defined(XPOWERS_CHIP_AXP2101)
      case XPOWERS_AXP2101: 
        AXP2101_showstatus();
        break;
#endif
#if defined(XPOWERS_CHIP_AXP202)
      case XPOWERS_AXP202:
        AXP202_showstatus();
        break;
#endif
    }
  }
}


float AXPxxx_getBatteryChargeCurrent()
{
  float res{};
  if (pmu != nullptr) {
    switch(pmu->getChipModel()) {
#if defined(XPOWERS_CHIP_AXP192)
      case XPOWERS_AXP192: return AXP192_getBatteryChargeCurrent();
#endif
#if defined(XPOWERS_CHIP_AXP2101)
      case XPOWERS_AXP2101: return AXP2101_getBatteryChargeCurrent();
#endif
#if defined(XPOWERS_CHIP_AXP202)
      case XPOWERS_AXP202: return AXP202_getBatteryChargeCurrent();
#endif
    }
  }
  return res;
}

float AXPxxx_getBattDischargeCurrent()
{
  float res{};
  if (pmu != nullptr) {
    switch(pmu->getChipModel()) {
#if defined(XPOWERS_CHIP_AXP192)
      case XPOWERS_AXP192: return AXP192_getBattDischargeCurrent();
#endif
#if defined(XPOWERS_CHIP_AXP2101)
      case XPOWERS_AXP2101: return AXP2101_getBattDischargeCurrent();
#endif
#if defined(XPOWERS_CHIP_AXP202)
      case XPOWERS_AXP202: return AXP202_getBattDischargeCurrent();
#endif
    }
  }
  return res;
}

float AXPxxx_getVbusCurrent()
{
  float res{};
  if (pmu != nullptr) {
    switch(pmu->getChipModel()) {
#if defined(XPOWERS_CHIP_AXP192)
      case XPOWERS_AXP192: return AXP192_getVbusCurrent();
#endif
#if defined(XPOWERS_CHIP_AXP2101)
      case XPOWERS_AXP2101: return AXP2101_getVbusCurrent();
#endif
#if defined(XPOWERS_CHIP_AXP202)
      case XPOWERS_AXP202: return AXP202_getVbusCurrent();
#endif
    }
  }
  return res;
}


void IRAM_ATTR PMUIRQ() { doIRQ(PMU_IRQ); }
#endif

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
  voltage = pmu->getBattVoltage();
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
  batt_percent = pmu->getBatteryPercent();
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
  if (pmu->isVbusIn())
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
