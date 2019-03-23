#ifndef _TIME_SYNC_TIMESERVER_H
#define _TIME_SYNC_TIMESERVER_H

#include <chrono>
#include "globals.h"
#include "timesync.h"
#include "timekeeper.h"

#define TIME_SYNC_SAMPLES 1   // number of time requests for averaging
#define TIME_SYNC_CYCLE 20    // delay between two time samples [seconds]
#define TIME_SYNC_TIMEOUT 120 // timeout waiting for timeserver answer [seconds]
#define TIME_SYNC_TRIGGER 100 // deviation triggering a time sync [milliseconds]
#define TIME_SYNC_FRAME_LENGTH 0x06 // timeserver answer frame length [bytes]
#define TIME_SYNC_FIXUP 30 // calibration to fixup processing time [milliseconds]

void send_timesync_req(void);
int recv_timesync_ans(uint8_t buf[], uint8_t buf_len);
void process_timesync_req(void *taskparameter);
void store_time_sync_req(uint32_t t_millisec);

#endif