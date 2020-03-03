#ifndef _TIMESYNC_H
#define _TIMESYNC_H

#include <chrono>
#include "globals.h"
#include "irqhandler.h"
#include "timekeeper.h"

//#define TIME_SYNC_TRIGGER 100 // threshold for time sync [milliseconds]
#define TIME_SYNC_FRAME_LENGTH 0x07 // timeserver answer frame length [bytes]
#define TIME_SYNC_FIXUP 4 // calibration to fixup processing time [milliseconds]
#define TIMEREQUEST_MAX_SEQNO 0xf0 // threshold for wrap around seqno

enum timesync_t {
  timesync_tx,
  timesync_rx,
  gwtime_sec,
  gwtime_msec,
  no_of_timestamps
};

void timesync_init(void);
void send_timesync_req(void);
int recv_timesync_ans(const uint8_t buf[], uint8_t buf_len);
void process_timesync_req(void *taskparameter);
void store_timestamp(uint32_t timestamp, timesync_t timestamp_type);

#endif
