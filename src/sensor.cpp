// Basic Config
#include "globals.h"

// Local logging tag
static const char TAG[] = "main";

uint8_t * sensor_read(uint8_t sensor) {

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