// clang-format off
// upload_speed 921600
// board m5stack-core-esp32

#ifndef _M5CORE_H
#define _M5CORE_H

#include <stdint.h>

#define HAS_LORA 1 // comment out if device shall not send data via LoRa or has no M5 RA01 LoRa module
#define LORA_SCK  SCK
#define LORA_CS   SS
#define LORA_MISO MISO
#define LORA_MOSI MOSI
#define LORA_RST  GPIO_NUM_26
#define LORA_IRQ  GPIO_NUM_36
#define LORA_IO1  GPIO_NUM_34 // must be wired by you on PCB!
#define LORA_IO2  LMIC_UNUSED_PIN

// enable only if you want to store a local paxcount table on the device
#define HAS_SDCARD  1      // this board has an SD-card-reader/writer
// Pins for SD-card
#define SDCARD_CS    GPIO_NUM_4
#define SDCARD_MOSI  MOSI
#define SDCARD_MISO  MISO
#define SDCARD_SCLK  SCK

#define CFG_sx1276_radio 1 // select LoRa chip
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_LED NOT_A_PIN // no on board LED (?)
#define HAS_BUTTON (39) // on board button A

// power management settings
#define HAS_IP5306 1 // has IP5306 chip
#define PMU_CHG_CURRENT 2 // battery charge current
// possible values: 0:200mA, 1:400mA, *2:500mA, 3:600mA
#define PMU_CHG_CUTOFF 0 // battery charge cutoff
// possible values: *0:4.2V, 1:4.3V, 2:4.35V, 3:4.4V

// Optional GPS Module settings
//#define HAS_GPS 1 // use GPS
//#define GPS_SERIAL 9600, SERIAL_8N1, GPIO_NUM_17, GPIO_NUM_16 // UBlox NEO M8N RX,TX
//#define GPS_INT GPIO_NUM_36 // 30ns accurary timepulse, to be external wired on pcb: shorten R12!

// Display Settings
#define HAS_DISPLAY 2   // TFT-LCD
//#define MY_DISPLAY_FLIP  1 // use if display is rotated
#define MY_DISPLAY_WIDTH 320
#define MY_DISPLAY_HEIGHT 240
#define TFT_MOSI MOSI   // SPI
#define TFT_MISO MISO   // SPI
#define TFT_SCLK SCK    // SPI
#define TFT_CS   GPIO_NUM_14    // Chip select control
#define TFT_DC   GPIO_NUM_27    // Data Command control
#define TFT_RST  GPIO_NUM_33    // Reset
#define TFT_BL   GPIO_NUM_32    // LED back-light
#define TFT_FREQUENCY 40000000
#define TFT_TYPE LCD_ILI9341, FLAGS_NONE, TFT_FREQUENCY, TFT_CS, TFT_DC, TFT_RST, TFT_BL, TFT_MISO, TFT_MOSI, TFT_SCLK

#endif
