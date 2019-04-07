#ifndef _TIMESYNC_H
#define _TIMESYNC_H

#include <chrono>
#include "globals.h"
#include "irqhandler.h"
#include "timekeeper.h"

//#define TIME_SYNC_TRIGGER 100 // threshold for time sync [milliseconds]
#define TIME_SYNC_FRAME_LENGTH 0x05 // timeserver answer frame length [bytes]
#define TIME_SYNC_FIXUP 6 // calibration to fixup processing time [milliseconds]

extern TaskHandle_t timeSyncReqTask;

void send_timesync_req(void);
int recv_timesync_ans(uint8_t seq_no, uint8_t buf[], uint8_t buf_len);
void process_timesync_req(void *taskparameter);
void store_time_sync_req(uint32_t t_millisec);
void IRAM_ATTR setMyTime(uint32_t t_sec, uint16_t t_msec);

#endif