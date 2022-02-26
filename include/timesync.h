#ifndef _TIMESYNC_H
#define _TIMESYNC_H

#include "globals.h"
#include "irqhandler.h"
#include "timekeeper.h"

#define TIME_SYNC_FRAME_LENGTH 6 // timeserver answer frame length [bytes]
#define TIME_SYNC_FIXUP 25 // compensation for processing time [milliseconds]
#define TIME_SYNC_MAX_SEQNO 0xfe // threshold for wrap around time_sync_seqNo
#define TIME_SYNC_END_FLAG (TIME_SYNC_MAX_SEQNO + 1) // end of handshake marker

enum timesync_t {
  timesync_tx,
  timesync_rx,
  gwtime_sec,
  gwtime_msec,
  no_of_timestamps
};

void timesync_init(void);
void timesync_request(void);
void timesync_store(uint32_t timestamp, timesync_t timestamp_type);
void timesync_processReq(void *taskparameter);
void timesync_serverAnswer(void *pUserData, int flag);

#endif
