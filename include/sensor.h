#ifndef _SENSOR_H
#define _SENSOR_H

#include "configmanager.h"

#define HAS_SENSORS (HAS_SENSOR_1 || HAS_SENSOR_2 || HAS_SENSOR_3)

uint8_t sensor_mask(uint8_t sensor_no);
uint8_t *sensor_read(uint8_t sensor);
void sensor_init(void);

#endif