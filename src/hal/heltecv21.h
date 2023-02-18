// clang-format off
// upload_speed 921600
// board heltec_wifi_lora_32_V2

#ifndef _HELTECV21_H
#define _HELTECV21_H

#include <stdint.h>

// Hardware related definitions for Heltec V2.1 LoRa-32 Board
// see https://docs.heltec.org/en/node/esp32/dev-board/hardware_update_log.html#v2-1

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1

#define HAS_DISPLAY 1 // OLED-Display on board
#define HAS_LED LED_BUILTIN                           // white LED on board (set to NOT_A_PIN to disable)
#define HAS_BUTTON KEY_BUILTIN                        // button "PROG" on board

#define BAT_MEASURE_ADC ADC1_GPIO37_CHANNEL  // battery probe GPIO pin
#define BAT_VOLTAGE_DIVIDER 2 // voltage divider 220k/100k on board

// switches battery power and Vext, switch logic 0 = on / 1 = off
#define EXT_POWER_SW Vext
#define EXT_POWER_ON 0

// Pins for I2C interface of OLED Display
#define MY_DISPLAY_SDA SDA_OLED
#define MY_DISPLAY_SCL SCL_OLED
#define MY_DISPLAY_RST RST_OLED

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
