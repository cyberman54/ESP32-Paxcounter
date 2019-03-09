#ifndef _TIME_SYNC_TIMESERVER_H
#define _TIME_SYNC_TIMESERVER_H

#include "globals.h"
#include "timesync.h"
#include "timekeeper.h"

#define TIME_SYNC_SAMPLES 3   // number of time requests for averaging
#define TIME_SYNC_CYCLE 20    // seconds between two time requests
#define TIME_SYNC_TIMEOUT 180 // timeout seconds waiting for timeserver answer
#define TIME_SYNC_THRESHOLD                                                    \
  1.0f // time deviation threshold triggering time sync
#define TIME_SYNC_START_OPCODE 0x90 // start time sync on node
#define TIME_SYNC_STOP_OPCODE 0x91  // stop time sync on node
#define TIME_SYNC_REQ_OPCODE 0x92   // node request at timeserver
#define TIME_SYNC_ANS_OPCODE 0x93   // timeserver answer to node

typedef struct {
  uint32_t seconds;
  uint8_t fractions; // 1/250ths second = 4 milliseconds resolution
} time_sync_message_t;

extern time_sync_message_t time_sync_messages[], time_sync_answers[];
extern uint8_t time_sync_seqNo;

void send_Servertime_req(void);
void recv_Servertime_ans(uint8_t val[]);
void process_Servertime_sync_req(void *taskparameter);
void process_Servertime_sync_ans(void *taskparameter);
void force_Servertime_sync(uint8_t val[]);
void store_time_sync_req(time_t secs, uint32_t micros);

#endif