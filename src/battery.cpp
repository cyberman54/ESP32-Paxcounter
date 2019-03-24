#include "globals.h"

// Local logging tag
static const char TAG[] = __FILE__;

#ifdef HAS_BATTERY_PROBE
esp_adc_cal_characteristics_t *adc_characs =
    (esp_adc_cal_characteristics_t *)calloc(
        1, sizeof(esp_adc_cal_characteristics_t));

static const adc1_channel_t adc_channel = BAT_MEASURE_ADC;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;
#endif

void calibrate_voltage(void) {
#ifdef HAS_BATTERY_PROBE
  // configure ADC
  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
  ESP_ERROR_CHECK(adc1_config_channel_atten(adc_channel, atten));
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

uint16_t read_voltage() {
#ifdef HAS_BATTERY_PROBE
  // multisample ADC
  uint32_t adc_reading = 0;
  for (int i = 0; i < NO_OF_SAMPLES; i++) {
    adc_reading += adc1_get_raw(adc_channel);
  }
  adc_reading /= NO_OF_SAMPLES;
  // Convert ADC reading to voltage in mV
  uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_characs);
#ifdef BAT_VOLTAGE_DIVIDER
  voltage *= BAT_VOLTAGE_DIVIDER;
#endif

#ifdef BAT_MEASURE_EN // turn ext. power off
  digitalWrite(EXT_POWER_SW, EXT_POWER_OFF);
#endif

  return (uint16_t)voltage;
#else
  return 0;
#endif
}

bool batt_sufficient() {
#ifdef BAT_MEASURE_ADC
  uint16_t volts = read_voltage();
  return ((volts < 1000) ||
          (volts > OTA_MIN_BATT)); // no battery or battery sufficient
#else
  return true;
#endif
}