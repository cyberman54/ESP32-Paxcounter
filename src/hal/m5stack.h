// clang-format off
// upload_speed 921600
// board M5Stack-Core-ESP32

// EXPERIMENTAL VERSION - NOT TESTED ON M5 HARDWARE YET

#ifndef _M5STACK_H
#define _M5STACK_H

#include <stdint.h>

#define HAS_LORA 1 // comment out if device shall not send data via LoRa or has no M5 RA01 LoRa module

// Pins for LORA chip SPI interface, reset line and interrupt lines
#define LORA_SCK  SCK
#define LORA_CS   SS
#define LORA_MISO MISO
#define LORA_MOSI MOSI
#define LORA_RST  GPIO_NUM_36
#define LORA_IRQ  GPIO_NUM_26
#define LORA_IO1  GPIO_NUM_34 // must be externally wired on PCB!
#define LORA_IO2  LMIC_UNUSED_PIN


// enable only if you want to store a local paxcount table on the device
#define HAS_SDCARD  1      // this board has an SD-card-reader/writer
// Pins for SD-card
#define SDCARD_CS    GPIO_NUM_4
#define SDCARD_MOSI  MOSI
#define SDCARD_MISO  MISO
#define SDCARD_SCLK  SCK

// user defined sensors
//#define HAS_SENSORS 1 // comment out if device has user defined sensors

#define CFG_sx1276_radio 1 // select LoRa chip
#define BOARD_HAS_PSRAM // use if board has external PSRAM
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

//#define HAS_DISPLAY 1
//#define DISPLAY_FLIP  1 // use if display is rotated
//#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
//#define BAT_VOLTAGE_DIVIDER 2 // voltage divider 100k/100k on board

#define HAS_LED NOT_A_PIN // no on board LED (?)
#define HAS_BUTTON (39) // on board button A

// GPS settings
#define HAS_GPS 1 // use on board GPS
#define GPS_SERIAL 9600, SERIAL_8N1, RXD2, TXD2 // UBlox NEO 6M RX, TX
#define GPS_INT GPIO_NUM_35 // 30ns accurary timepulse, to be external wired on pcb: shorten R12!

// Pins for interface of LC Display
#define MY_OLED_CS GPIO_NUM_14
#define MY_OLED_DC GPIO_NUM_27
#define MY_OLED_CLK GPIO_NUM_18
#define MY_OLED_RST GPIO_NUM_33
#define MY_OLED_BL GPIO_NUM_32
#define MY_OLED_MOSI GPIO_NUM_23
#define MY_OLED_MISO GPIO_NUM_23

#endif