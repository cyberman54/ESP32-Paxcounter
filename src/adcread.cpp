#ifdef HAS_BATTERY_PROBE

/* ADC1 Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "globals.h"

#include <driver/adc.h>
#include <esp_adc_cal.h>

#define DEFAULT_VREF    1100    // optional use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          // we do multisampling

// Local logging tag
static const char TAG[] = "main";

#ifdef VERBOSE

    static void check_efuse()
    {
        //Check if two point calibration values are burned into eFuse
        if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
            ESP_LOGI(TAG,"eFuse Two Point: Supported");
        } else {
            ESP_LOGI(TAG,"eFuse Two Point: NOT supported");
        }

        //Check Vref is burned into eFuse
        if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
            ESP_LOGI(TAG,"eFuse Vref: Supported");
        } else {
            ESP_LOGI(TAG,"eFuse Vref: NOT supported");
        }
    }

    static void print_char_val_type(esp_adc_cal_value_t val_type)
    {
        if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
            ESP_LOGI(TAG,"Characterized using Two Point Value");
        } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
            ESP_LOGI(TAG,"Characterized using eFuse Vref");
        } else {
            ESP_LOGI(TAG,"Characterized using Default Vref");
        }
    }

#endif // verbose

uint16_t read_voltage(void)
{
    static const adc1_channel_t channel = HAS_BATTERY_PROBE;
    static const adc_atten_t atten = ADC_ATTEN_DB_11;
    static const adc_unit_t unit = ADC_UNIT_1;

    #ifdef VERBOSE
        //show if Two Point or Vref are burned into eFuse
        check_efuse();
    #endif

    //Configure ADC1
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten(channel, atten));

    //Characterize ADC1
    esp_adc_cal_characteristics_t *adc_chars = (esp_adc_cal_characteristics_t *) calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    
    #ifdef VERBOSE
        //show calibration source
        print_char_val_type(val_type);
    #endif

    //sample ADC1
    uint32_t adc_reading = 0;
    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
            adc_reading += adc1_get_raw(channel);
    }

    adc_reading /= NO_OF_SAMPLES;

    //Convert adc_reading to voltage in mV
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    #ifdef BATT_FACTOR
        voltage *= BATT_FACTOR;
    #endif
    ESP_LOGI(TAG,"Raw: %d / Voltage: %dmV", adc_reading, voltage);
    return voltage;
}
#endif // HAS_BATTERY_PROBE