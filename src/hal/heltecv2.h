// clang-format off

#ifndef _HELTECV2_H
#define _HELTECV2_H

#include <stdint.h>

// Hardware related definitions for Heltec V2 LoRa-32 Board

// BME680 sensor on I2C bus (SDI=21/SCL=22); comment out if not present
//#define HAS_BME GPIO_NUM_21, GPIO_NUM_22 // SDA, SCL
//#define BME_ADDR BME680_I2C_ADDR_PRIMARY // connect SDIO of BME680 to GND

// Of cause, by default the board has no BME680 mounted 
// a BME680 sensor board maybe connected to I2C (SDA = 4 , and SLC = 15)
// second it worked if SDIO left unconnected and 0x77 as address was used
//#define HAS_BME 4, 15 // SDA, SCL
//#define BME_ADDR BME680_I2C_ADDR_SECONDARY // leave SDIO of BME680 unconnected
 

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // OLED-Display on board
#define HAS_LED LED_BUILTIN                           // white LED on board
#define HAS_BUTTON KEY_BUILTIN                        // button "PROG" on board

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA (4)    // original = 21
#define MY_OLED_SCL (15)    // original = 22
#define MY_OLED_RST (16)

// Pins for LORA chip SPI interface come from board file, we need some
// additional definitions for LMIC
#define LORA_IO1  (35)
#define LORA_IO2  (34)

#endif
