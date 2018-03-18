#ifdef LOPY

#include <Arduino.h>

// Local logging tag
static const char *TAG = "antenna";

static antenna_type_t antenna_type_selected = ANTENNA_TYPE_INTERNAL;

void antenna_init(void) {
        gpio_config_t gpioconf = {.pin_bit_mask = 1ull << PIN_ANTENNA_SWITCH,
                                  .mode = GPIO_MODE_OUTPUT,
                                  .pull_up_en = GPIO_PULLUP_DISABLE,
                                  .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                  .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&gpioconf);
}

void antenna_select (antenna_type_t _antenna) {
        if (PIN_ANTENNA_SWITCH < 32) {
            // set the pin value
            if (_antenna == ANTENNA_TYPE_EXTERNAL) {
                GPIO_REG_WRITE(GPIO_OUT_W1TS_REG, 1 << PIN_ANTENNA_SWITCH);
            } else {
                GPIO_REG_WRITE(GPIO_OUT_W1TC_REG, 1 << PIN_ANTENNA_SWITCH);
            }
        } else {
            if (_antenna == ANTENNA_TYPE_EXTERNAL) {
                GPIO_REG_WRITE(GPIO_OUT1_W1TS_REG, 1 << (PIN_ANTENNA_SWITCH & 31));
            } else {
                GPIO_REG_WRITE(GPIO_OUT1_W1TC_REG, 1 << (PIN_ANTENNA_SWITCH & 31));
            }
        }
        antenna_type_selected = _antenna;
}

#endif //