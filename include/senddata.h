#ifndef _SENDDATA_H
#define _SENDDATA_H

#include "spislave.h"
#include "lorawan.h"
#include "cyclic.h"

void SendPayload(uint8_t port);
void sendCounter(void);
void checkSendQueues(void);
void flushQueues();

#endif // _SENDDATA_H_