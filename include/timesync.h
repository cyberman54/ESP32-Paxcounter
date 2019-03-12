#ifndef _TIME_SYNC_TIMESERVER_H
#define _TIME_SYNC_TIMESERVER_H

#include <ctime>
#include <chrono>
#include "globals.h"
#include "timesync.h"
#include "timekeeper.h"

#define TIME_SYNC_SAMPLES 3   // number of time requests for averaging
#define TIME_SYNC_CYCLE 20    // seconds between two time requests
#define TIME_SYNC_TIMEOUT 120 // timeout seconds waiting for timeserver answer
#define TIME_SYNC_TRIGGER 1   // time deviation threshold triggering time sync
#define TIME_SYNC_FRAME_LENGTH 0x06 // timeserver answer frame length

void send_timesync_req(void);
void recv_timesync_ans(uint8_t buf[], uint8_t buf_len);
void process_timesync_req(void *taskparameter);
void store_time_sync_req(time_t secs, uint32_t micros);

#endif