#ifndef _SENDDATA_H
#define _SENDDATA_H

#include "spislave.h"
#include "lorawan.h"
#include "cyclic.h"

void SendData(uint8_t port);
void sendPayload(void);
void checkSendQueues(void);
void flushQueues();

#endif // _SENDDATA_H_