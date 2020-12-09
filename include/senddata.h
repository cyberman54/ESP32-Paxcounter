#ifndef _SENDDATA_H
#define _SENDDATA_H

#include "spislave.h"
#include "mqttclient.h"
#include "cyclic.h"
#include "sensor.h"
#include "lorawan.h"
#include "display.h"
#include "sdcard.h"

#if (COUNT_ENS)
#include "corona.h"
#endif

extern Ticker sendTimer;

void SendPayload(uint8_t port, sendprio_t prio);
void sendData(void);
void checkSendQueues(void);
void flushQueues(void);
bool allQueuesEmtpy(void);
void setSendIRQ(void);

#endif // _SENDDATA_H_
