#ifndef _HOMECYCLE_H
#define _HOMECYCLE_H

void doHomework(void);
void checkHousekeeping(void);
void homeCycleIRQ(void);
uint64_t uptime(void);
void reset_counters(void);
int redirect_log(const char *fmt, va_list args);

#endif