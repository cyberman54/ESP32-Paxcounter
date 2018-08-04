#ifndef _SENDDATA_H
#define _SENDDATA_H

// Struct holding payload for data send queue
typedef struct {
  uint8_t MessageSize;
  uint8_t MessagePort;
  uint8_t Message[PAYLOAD_BUFFER_SIZE];
} MessageBuffer_t;

void EnqueueSendData(uint8_t port, uint8_t data[], uint8_t size);
void sendPayload(void);
void SendCycleIRQ(void);
void processSendBuffer(void);

#endif // _SENDDATA_H_