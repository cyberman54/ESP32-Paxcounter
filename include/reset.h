#ifndef _RESET_H
#define _RESET_H

#include <driver/rtc_io.h>
#include <soc/reset_reasons.h>

#include "i2c.h"
#include "lorawan.h"
#include "display.h"
#include "power.h"
#include "sdcard.h"
#include "sds011read.h"

void reset_rtc_vars(void);
void do_reset(bool warmstart);
void do_after_reset(void);
void enter_deepsleep(const uint32_t wakeup_sec, const gpio_num_t wakeup_gpio);
unsigned long long uptime(void);

enum runmode_t {
  RUNMODE_POWERCYCLE,
  RUNMODE_NORMAL,
  RUNMODE_WAKEUP,
  RUNMODE_UPDATE,
  RUNMODE_SLEEP,
  RUNMODE_MAINTENANCE
};

extern RTC_NOINIT_ATTR runmode_t RTC_runmode;
extern RTC_NOINIT_ATTR uint32_t RTC_restarts;

#endif // _RESET_H