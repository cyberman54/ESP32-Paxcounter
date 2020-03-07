#ifndef _TIMESYNC_H
#define _TIMESYNC_H

#include "globals.h"
#include "irqhandler.h"
#include "timekeeper.h"

#define TIME_SYNC_FRAME_LENGTH 0x07 // timeserver answer frame length [bytes]
#define TIME_SYNC_FIXUP 16 // compensation for processing time [milliseconds]
#define TIMEREQUEST_MAX_SEQNO 0xfe // threshold for wrap around seqno
#define TIMEREQUEST_END                                                        \
  (TIMEREQUEST_MAX_SEQNO + 1) // marker for end of timesync handshake
#define GPS_UTC_DIFF 315964800

enum timesync_t {
  timesync_tx,
  timesync_rx,
  gwtime_sec,
  gwtime_msec,
  no_of_timestamps
};

void timesync_init(void);
void timesync_sendReq(void);
void timesync_storeReq(uint32_t timestamp, timesync_t timestamp_type);
void IRAM_ATTR timesync_processReq(void *taskparameter);
void IRAM_ATTR timesync_serverAnswer(void *pUserData, int flag);

#endif
