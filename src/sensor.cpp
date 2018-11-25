// Basic Config
#include "globals.h"

// Local logging tag
static const char TAG[] = "main";

#define SENSORBUFFER 10 // max. size of user sensor data buffer in bytes [default=20]

void sensor_init(void) {

  // this function is called dureing device startup
  // put your sensor initialization routines here
}

uint8_t sensor_mask(uint8_t sensor_no) {
  switch (sensor_no) {
  case 1:
    return (uint8_t)SENSOR1_DATA;
  case 2:
    return (uint8_t)SENSOR2_DATA;
    break;
  case 3:
    return (uint8_t)SENSOR3_DATA;
  case 4:
    return (uint8_t)SENSOR4_DATA;
  }
}

uint8_t *sensor_read(uint8_t sensor) {

  static uint8_t buf[SENSORBUFFER] = {0};
  uint8_t length = 3;

  switch (sensor) {

  case 1:

    buf[0] = length;
    buf[1] = 0xff;
    buf[2] = 0xa0;
    buf[3] = 0x01;
    break;

  case 2:

    buf[0] = length;
    buf[1] = 0xff;
    buf[2] = 0xa0;
    buf[3] = 0x02;
    break;

  case 3:

    buf[0] = length;
    buf[1] = 0xff;
    buf[2] = 0xa0;
    buf[3] = 0x03;
    break;

  case 4:

    buf[0] = length;
    buf[1] = 0xff;
    buf[2] = 0xa0;
    buf[3] = 0x04;
    break;
  }

  return buf;
}