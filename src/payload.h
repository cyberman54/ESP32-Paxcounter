
#ifndef _PAYLOAD_H_
#define _PAYLOAD_H_

#include <Arduino.h>
#include "LoraEncoder.h"

// MyDevices CayenneLPP channels
#define LPP_GPS_CHANNEL 20
#define LPP_COUNT_WIFI_CHANNEL 21
#define LPP_COUNT_BLE_CHANNEL 22
#define LPP_BATT_CHANNEL 23
#define LPP_ADR_CHANNEL 25
#define LPP_TEMP_CHANNEL 26
// MyDevices CayenneLPP types
#define LPP_GPS 136          // 3 byte lon/lat 0.0001 °, 3 bytes alt 0.01m
#define LPP_TEMPERATURE 103  // 2 bytes, 0.1°C signed
#define LPP_DIGITAL_INPUT 0  // 1 byte
#define LPP_DIGITAL_OUTPUT 1 // 1 byte
#define LPP_ANALOG_INPUT 2   // 2 bytes, 0.01 signed
#define LPP_LUMINOSITY 101   // 2 bytes, 1 lux unsigned

class TTNplain {
public:
  TTNplain(uint8_t size);
  ~TTNplain();

  void reset(void);
  uint8_t getSize(void);
  uint8_t *getBuffer(void);

  void addCount(uint16_t value1, uint16_t value2);
  void addConfig(configData_t value);
  void addStatus(uint16_t voltage, uint64_t uptime, float cputemp);
#ifdef HAS_GPS
  void addGPS(gpsStatus_t value);
#endif

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

  void addCount(uint16_t value1, uint16_t value2);
  void addConfig(configData_t value);
  void addStatus(uint16_t voltage, uint64_t uptime, float cputemp);
#ifdef HAS_GPS
  void addGPS(gpsStatus_t value);
#endif

private:
  uint8_t *buffer;
  LoraEncoder message(byte *buffer);
};

class CayenneLPP {
public:
  CayenneLPP(uint8_t size);
  ~CayenneLPP();

  void reset(void);
  uint8_t getSize(void);
  uint8_t *getBuffer(void);

  void addCount(uint16_t value1, uint16_t value2);
  void addConfig(configData_t value);
  void addStatus(uint16_t voltage, uint64_t uptime, float cputemp);
#ifdef HAS_GPS
  void addGPS(gpsStatus_t value);
#endif

private:
  uint8_t *buffer;
  uint8_t maxsize;
  uint8_t cursor;
};

#endif // _PAYLOAD_H_
