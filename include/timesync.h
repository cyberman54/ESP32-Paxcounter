#ifndef _TIME_SYNC_TIMESERVER_H
#define _TIME_SYNC_TIMESERVER_H

#include "globals.h"
#include "timesync.h"
#include "timekeeper.h"

#define TIME_SYNC_SAMPLES 1    // number of time requests for averaging
#define TIME_SYNC_CYCLE 30     // seconds between two time requests
#define TIME_SYNC_TIMEOUT 120  // timeout seconds waiting for timeserver answer
#define TIME_SYNC_TRIGGER 1.0f // time deviation threshold triggering time sync
#define TIME_SYNC_FRAME_LENGTH 0x06 // timeserver answer frame length

typedef struct {
  uint32_t seconds;
  uint8_t fractions; // 1/250ths second = 4 milliseconds resolution
} time_sync_message_t;

void send_Servertime_req(void);
void recv_Servertime_ans(uint8_t buf[], uint8_t buf_len);
void process_Servertime_sync_req(void *taskparameter);
void store_time_sync_req(time_t secs, uint32_t micros);

#endif