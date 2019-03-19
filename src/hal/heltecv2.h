// clang-format off

#ifndef _HELTECV2_H
#define _HELTECV2_H

#include <stdint.h>

// Hardware related definitions for Heltec V2 LoRa-32 Board

//#define HAS_BME 1 // Enable BME sensors in general
//#define HAS_BME680 GPIO_NUM_4, GPIO_NUM_15 // SDA, SCL
//#define BME680_ADDR BME680_I2C_ADDR_PRIMARY // connect SDIO of BME680 to GND 

#define HAS_IF482 9600, SERIAL_7E1, GPIO_NUM_12, GPIO_NUM_13 // IF482 serial port parameters

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // OLED-Display on board
#define HAS_LED LED_BUILTIN                           // white LED on board
#define HAS_BUTTON KEY_BUILTIN                        // button "PROG" on board

//#define HAS_BATTERY_PROBE ADC2_GPIO13_CHANNEL  // battery probe GPIO pin
//#define BATT_FACTOR 4 // voltage divider 220k/100k on board
//#define HAS_LOWPOWER_SWITCH GPIO_NUM_21 // switches battery power, Vext control 0 = on / 1 = off
//#define LOW_POWER_ACTIVE_LOW 1

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA (4)
#define MY_OLED_SCL (15)
#define MY_OLED_RST (16)

// Pins for LORA chip SPI interface come from board file, we need some
// additional definitions for LMIC
#define LORA_IO1  (35)
#define LORA_IO2  (34)

#endif
