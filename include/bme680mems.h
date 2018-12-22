#ifndef _BME680MEMS_H
#define _BME680MEMS_H

#include "globals.h"
#include <Wire.h>
#include "bsec_integration.h"
#include "irqhandler.h"

extern const uint8_t bsec_config_iaq[454];

extern bmeStatus_t
    bme_status; // Make struct for storing gps data globally available
extern TaskHandle_t BmeTask;

int bme_init();
void bme_loop(void *pvParameters);
int8_t i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,
                uint16_t len);
int8_t i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,
                 uint16_t len);
void output_ready(int64_t timestamp, float iaq, uint8_t iaq_accuracy,
                  float temperature, float humidity, float pressure,
                  float raw_temperature, float raw_humidity, float gas,
                  bsec_library_return_t bsec_status, float static_iaq,
                  float co2_equivalent, float breath_voc_equivalent);
uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer);
void state_save(const uint8_t *state_buffer, uint32_t length);
uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer);
void user_delay_ms(uint32_t period);
int64_t get_timestamp_us();

#endif