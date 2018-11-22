#ifndef _BME680MEMS_H
#define _BME680MEMS_H

#include "globals.h"
#include <Wire.h>
#include "bme680.h"
#include "bsec_interface.h"
#include "bsec_datatypes.h"
//#include "bme680_defs.h"
//#include "bme680.c"

extern bmeStatus_t
    bme_status; // Make struct for storing gps data globally available

void bme_init();
bool bme_read();
void user_delay_ms(uint32_t period);
int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,
                     uint16_t len);
int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,
                      uint16_t len);

#endif