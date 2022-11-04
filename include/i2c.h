#ifndef _I2C_H
#define _I2C_H

#include <Arduino.h>
#include <Wire.h>
#include <BitBang_I2C.h>
#include <Wire.h>

#ifndef MY_DISPLAY_SDA
#define MY_DISPLAY_SDA SDA
#endif

#ifndef MY_DISPLAY_SCL
#define MY_DISPLAY_SCL SCL
#endif

#define SSD1306_PRIMARY_ADDRESS (0x3D)
#define SSD1306_SECONDARY_ADDRESS (0x3C)
#define BME_PRIMARY_ADDRESS (0x77)
#define BME_SECONDARY_ADDRESS (0x76)
#define AXP192_PRIMARY_ADDRESS (0x34)
#define IP5306_PRIMARY_ADDRESS (0x75)
#define MCP_24AA02E64_PRIMARY_ADDRESS (0x50)
#define QUECTEL_GPS_PRIMARY_ADDRESS (0x10)

extern SemaphoreHandle_t I2Caccess;

void i2c_init(void);
void i2c_deinit(void);
void i2c_scan(void);
int i2c_readBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);
int i2c_writeBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);

#endif