#ifndef _I2C_H
#define _I2C_H

#include <Arduino.h>

#define SSD1306_PRIMARY_ADDRESS (0x3D)
#define SSD1306_SECONDARY_ADDRESS (0x3C)
#define BME_PRIMARY_ADDRESS (0x77)
#define BME_SECONDARY_ADDRESS (0x76)
#define AXP192_PRIMARY_ADDRESS (0x34)
#define MCP_24AA02E64_PRIMARY_ADDRESS (0x50)
#define QUECTEL_GPS_PRIMARY_ADDRESS (0x10)

void i2c_init(void);
void i2c_deinit(void);
int i2c_scan(void);
uint8_t i2c_readBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);
uint8_t i2c_writeBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);

#endif