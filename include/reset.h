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
void enter_deepsleep(const uint64_t wakeup_sec, const gpio_num_t wakeup_gpio);

extern RTC_DATA_ATTR unsigned long RTC_millis;

#endif // _RESET_H