
#ifndef _PAYLOAD_H_
#define _PAYLOAD_H_

#include <Arduino.h>
#include "LoraEncoder.h"

class TTNplain {
public:
  TTNplain(uint8_t size);
  ~TTNplain();

  void reset(void);
  uint8_t getSize(void);
  uint8_t *getBuffer(void);

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

class TTNserialized {
public:
  TTNserialized(uint8_t size);
  ~TTNserialized();

  void reset(void);
  uint8_t getSize(void);
  uint8_t *getBuffer(void);

  // application payloads
  void addCount(uint16_t value1, uint16_t value2);
  void addGPS(gpsStatus_t value);

  // payloads for get rcommands
  void addConfig(configData_t value);
  void addStatus(uint16_t voltage, uint64_t uptime, float cputemp);

private:
  uint8_t *buffer;
  LoraEncoder message(byte *buffer);
};

#endif
