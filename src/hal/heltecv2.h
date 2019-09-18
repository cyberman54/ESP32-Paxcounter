// clang-format off
// upload_speed 921600
// board heltec_wifi_lora_32_V2

#ifndef _HELTECV2_H
#define _HELTECV2_H

#include <stdint.h>

// Hardware related definitions for Heltec V2 LoRa-32 Board

//#define HAS_BME 1 // Enable BME sensors in general
//#define HAS_BME680 GPIO_NUM_4, GPIO_NUM_15 // SDA, SCL
//#define BME680_ADDR BME680_I2C_ADDR_PRIMARY // connect SDIO of BME680 to GND 

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // OLED-Display on board
#define HAS_LED LED_BUILTIN                           // white LED on board
#define HAS_BUTTON KEY_BUILTIN                        // button "PROG" on board

// caveat: activating ADC2 conflicts with Wifi in current arduino-esp32
// see https://github.com/espressif/arduino-esp32/issues/3222
// thus we must waiver of battery monitoring 
//#define BAT_MEASURE_ADC ADC2_GPIO13_CHANNEL  // battery probe GPIO pin
//#define BAT_MEASURE_ADC_UNIT 2 // ADC 2
//#define BAT_VOLTAGE_DIVIDER 2 // voltage divider 220k/100k on board

#define EXT_POWER_SW Vext // switches battery power, Vext control 0 = on / 1 = off
#define EXT_POWER_ON    0
//#define EXT_POWER_OFF   1

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA SDA_OLED
#define MY_OLED_SCL SCL_OLED
#define MY_OLED_RST RST_OLED

// Pins for LORA chip SPI interface come from board file, we need some
// additional definitions for LMIC
#define LORA_IRQ DIO0
#define LORA_IO1 DIO1
#define LORA_IO2 DIO2
#define LORA_SCK SCK
#define LORA_MISO MISO
#define LORA_MOSI MOSI
#define LORA_RST RST_LoRa
#define LORA_CS SS

#endif
