// clang-format off
// upload_speed 921600
// board lolin32

//Copied from lolin32lite and changed for TTGO5 (changes at bottom)

#ifndef _TTGO5
#define _TTGO5

#include <stdint.h>

// Hardware related definitions for lolin32lite (without LoRa shield)

//#define HAS_LED LED_BUILTIN // on board LED on GPIO5

#define HAS_BUTTON GPIO_NUM_39 //  on board button



// when activated inverse behavior of LED
//#define LED_ACTIVE_LOW 1    // Onboard LED is active when pin is LOW

#define MY_SDA SDA
#define MY_SCL SCL

// disable brownout detection (avoid unexpected reset on some boards)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

// from here copied from ttgov21new.h
#define HAS_SDCARD 1     // // sPI

// Pins for SPI interface
#define SDCARD_CS   (13) // fill in the correct numbers for your board
#define SDCARD_MOSI (15)
#define SDCARD_MISO (2)
#define SDCARD_SCLK (14)

//gpio_pulldown_dis(13)
//gpio_pullup_en(13)


//#define HAS_DISPLAY 1
#define HAS_LED (19) // green on board LED

// WAS wurde von mir deaktiviert um das Absturzproblem zu beheben
#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BAT_VOLTAGE_DIVIDER 2 // voltage divider 100k/100k on board

// Pins for I2C interface of OLED Display
//#define MY_DISPLAY_SDA (21)  // 21
//#define MY_DISPLAY_SCL (22)  // 22

#define MY_DISPLAY_RST (16)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#define HAS_DISPLAY 3



#endif