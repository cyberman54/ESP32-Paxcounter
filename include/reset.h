#ifndef _RESET_H
#define _RESET_H

#include <driver/rtc_io.h>
#include <rom/rtc.h>

#include "i2c.h"
#include "lorawan.h"
#include "display.h"
#include "power.h"

void do_reset(bool warmstart);
void do_after_reset(void);
void enter_deepsleep(const uint64_t wakeup_sec = 60,
                     const gpio_num_t wakeup_gpio = GPIO_NUM_MAX);
unsigned long long uptime(void);

extern RTC_NOINIT_ATTR runmode_t RTC_runmode;

#endif // _RESET_H