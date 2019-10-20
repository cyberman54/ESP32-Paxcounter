#ifndef _RESET_H
#define _RESET_H

#include <driver/rtc_io.h>
#include <rom/rtc.h>
#include "i2c.h"

void do_reset(bool warmstart);
void do_after_reset(int reason);
void enter_deepsleep(const int wakeup_sec, const gpio_num_t wakeup_gpio);

#endif // _RESET_H