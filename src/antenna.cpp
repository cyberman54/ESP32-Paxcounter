/* switches wifi antenna, if board has switch to select internal and external antenna */

#ifdef HAS_ANTENNA_SWITCH

#include <Arduino.h>

// Local logging tag
static const char *TAG = "antenna";

typedef enum {
    ANTENNA_INT = 0,
    ANTENNA_EXT
} antenna_type_t;

void antenna_init(void) {
        gpio_config_t gpioconf = {.pin_bit_mask = 1ull << HAS_ANTENNA_SWITCH,
                                  .mode = GPIO_MODE_OUTPUT,
                                  .pull_up_en = GPIO_PULLUP_DISABLE,
                                  .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                  .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&gpioconf);
}

void antenna_select (const uint8_t _ant) {
        if (HAS_ANTENNA_SWITCH < 32) {
            if (_ant == ANTENNA_EXT) {
                GPIO_REG_WRITE(GPIO_OUT_W1TS_REG, 1 << HAS_ANTENNA_SWITCH);
            } else {
                GPIO_REG_WRITE(GPIO_OUT_W1TC_REG, 1 << HAS_ANTENNA_SWITCH);
            }
        } else {
            if (_ant == ANTENNA_EXT) {
                GPIO_REG_WRITE(GPIO_OUT1_W1TS_REG, 1 << (HAS_ANTENNA_SWITCH & 31));
            } else {
                GPIO_REG_WRITE(GPIO_OUT1_W1TC_REG, 1 << (HAS_ANTENNA_SWITCH & 31));
            }
        }
        ESP_LOGI(TAG, "Wifi Antenna switched to %s", _ant ? "external" : "internal");
}

#endif