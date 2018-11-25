#ifndef _BME680MEMS_H
#define _BME680MEMS_H

#include "globals.h"
#include <Wire.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "bsec_integration.h"
#include "bsec_integration.c"

extern bmeStatus_t
    bme_status; // Make struct for storing gps data globally available

int bme_init();
bool bme_read();
void user_delay_ms(uint32_t period);
int64_t get_timestamp_us();

int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,
                     uint16_t len);
int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,
                      uint16_t len);

uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer);
void state_save(const uint8_t *state_buffer, uint32_t length);
uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer);


#endif