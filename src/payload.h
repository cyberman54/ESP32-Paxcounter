
#ifndef _PAYLOAD_H_
#define _PAYLOAD_H_

#include <Arduino.h>

#define TTN_PAYLOAD_COUNTER 0
#define TTN_PAYLOAD_GPS 4
#define TTN_PAYLOAD_CONFIG 0
#define TTN_PAYLOAD_STATUS 0

class TTNplain {
public:
  TTNplain(uint8_t size);
  ~TTNplain();

  void reset(void);
  uint8_t getSize(void);
  uint8_t *getBuffer(void);
  uint8_t copy(uint8_t *buffer);

  // application payloads
  void addCount(uint16_t value1, uint16_t value2);
  void addGPS(gpsStatus_t value);

  // payloads for get rcommands
  void addConfig(configData_t value);
  void addStatus(uint16_t voltage, uint64_t uptime, float cputemp);

private:
  uint8_t *buffer;
  uint8_t maxsize;
  uint8_t cursor;
};

#endif