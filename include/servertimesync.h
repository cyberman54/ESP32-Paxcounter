#ifndef _ServertimeSYNC_H
#define _ServertimeSYNC_H

#include "globals.h"
#include "Servertimesync.h"
#include "timekeeper.h"

#define SYNC_SAMPLES 3
//#define SYNC_CYCLE 600 // seconds between two time sync requests
#define SYNC_CYCLE 20 // seconds between two time sync requests
#define SYNC_TIMEOUT                                                           \
  (SYNC_SAMPLES * (SYNC_CYCLE + 60)) // timeout waiting for time sync answer
#define SYNC_THRESHOLD 0.01f // time deviation threshold triggering time sync
#define TIME_SYNC_OPCODE 0x90
#define TIME_REQ_OPCODE 0x92
#define TIME_ANS_OPCODE 0x93

extern uint32_t time_sync_messages[], time_sync_answers[];
extern uint8_t time_sync_seqNo;

void send_Servertime_req(void);
void recv_Servertime_ans(uint8_t val[]);
void process_Servertime_sync_req(void *taskparameter);
void process_Servertime_sync_ans(void *taskparameter);
void force_Servertime_sync(uint8_t val[]);

#endif