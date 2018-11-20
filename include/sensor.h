#ifndef _SENSOR_H
#define _SENSOR_H

uint8_t sensor_mask(uint8_t sensor_no);
uint8_t * sensor_read(uint8_t sensor);
void sensor_init(void);

#endif