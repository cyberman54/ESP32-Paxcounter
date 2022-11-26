#ifndef _SENDDATA_H
#define _SENDDATA_H

#include "spislave.h"
#include "mqttclient.h"
#include "cyclic.h"
#include "sensor.h"
#include "lorawan.h"
#include "display.h"
#include "sdcard.h"
#include "payload.h"

void SendPayload(uint8_t port);
void sendData(void);
void checkSendQueues(void);
void flushQueues(void);
bool allQueuesEmtpy(void);
void setSendIRQ(TimerHandle_t xTimer);
void setSendIRQ(void);

#endif // _SENDDATA_H_
