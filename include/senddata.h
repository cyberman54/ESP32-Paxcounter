#ifndef _SENDDATA_H
#define _SENDDATA_H

#include "libpax_helpers.h"
#include "spislave.h"
#include "mqttclient.h"
#include "cyclic.h"
#include "sensor.h"
#include "lorawan.h"
#include "display.h"
#include "sdcard.h"

extern struct count_payload_t count_from_libpax;

void SendPayload(uint8_t port);
void sendData(void);
void checkSendQueues(void);
void flushQueues(void);
bool allQueuesEmtpy(void);
void setSendIRQ(TimerHandle_t xTimer = NULL);
void initSendDataTimer(uint8_t sendcycle);

#endif // _SENDDATA_H_
