// clang-format off

#ifndef _TTGOFOX_H
#define _TTGOFOX_H

#include <stdint.h>

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1 // HPD13A LoRa SoC

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
#define HAS_LED NOT_A_PIN // green on board LED is useless, is GPIO25, which switches power for Lora+Display

//#define EXT_POWER_SW GPIO_NUM_25 // switches power for LoRa chip + display (0 = off / 1 = on)
//#define EXT_POWER_ON    1
//#define EXT_POWER_OFF   0
#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL
#define BAT_VOLTAGE_DIVIDER 2 // voltage divider 100k/100k on board
#define HAS_BUTTON GPIO_NUM_36 // on board button (next to reset)

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA (21)
#define MY_OLED_SCL (22)
#define MY_OLED_RST U8X8_PIN_NONE

// Settings for on board DS3231 RTC chip
#define HAS_RTC MY_OLED_SDA, MY_OLED_SCL // SDA, SCL
#define RTC_INT GPIO_NUM_34 // timepulse with accuracy +/- 2*e-6 [microseconds] = 0,1728sec / day

// Settings for IF482 interface
#define HAS_IF482 9600, SERIAL_7E1, GPIO_NUM_12, GPIO_NUM_14 // IF482 serial port parameters

// Settings for DCF77 interface
//#define HAS_DCF77 GPIO_NUM_14
//#define DCF77_ACTIVE_LOW 1

// Pins for LORA chip SPI interface, reset line and interrupt lines
#define LORA_SCK  SCK 
#define LORA_CS   SS
#define LORA_MISO MISO
#define LORA_MOSI MOSI
#define LORA_RST  (23)
#define LORA_IRQ  (26)
#define LORA_IO1  (33)
#define LORA_IO2  (32)

#endif
