#ifdef HAS_BATTERY_PROBE

#include "globals.h"

// Local logging tag
static const char TAG[] = "main";

static void print_char_val_type(esp_adc_cal_value_t val_type) {
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    ESP_LOGD(TAG,
             "ADC characterization based on Two Point values stored in eFuse");
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    ESP_LOGD(TAG,
             "ADC characterization based on reference voltage stored in eFuse");
  } else {
    ESP_LOGD(TAG, "ADC characterization based on default reference voltage");
  }
}

uint16_t read_voltage(void) {
  static const adc1_channel_t channel = HAS_BATTERY_PROBE;
  static const adc_atten_t atten = ADC_ATTEN_DB_11;
  static const adc_unit_t unit = ADC_UNIT_1;

  // configure ADC1
  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
  ESP_ERROR_CHECK(adc1_config_channel_atten(channel, atten));

  // calibrate ADC1
  esp_adc_cal_characteristics_t *adc_chars =
      (esp_adc_cal_characteristics_t *)calloc(
          1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
      unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
  print_char_val_type(val_type);

  // multisample ADC1
  uint32_t adc_reading = 0;
  for (int i = 0; i < NO_OF_SAMPLES; i++) {
    adc_reading += adc1_get_raw(channel);
  }

  adc_reading /= NO_OF_SAMPLES;

  // Convert adc_reading to voltage in mV
  uint16_t voltage =
      (uint16_t)esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
#ifdef BATT_FACTOR
  voltage *= BATT_FACTOR;
#endif
  ESP_LOGI(TAG, "Raw: %d / Voltage: %dmV", adc_reading, voltage);
  return voltage;
}
#endif // HAS_BATTERY_PROBE