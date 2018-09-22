#ifndef _SENDDATA_H
#define _SENDDATA_H

void SendData(uint8_t port);
void sendPayload(void);
void SendCycleIRQ(void);
void enqueuePayload(void);
void flushQueues();

#endif // _SENDDATA_H_