#ifndef _PAYLOAD_H_
#define _PAYLOAD_H_

// MyDevices CayenneLPP channels for dynamic sensor payload format
#if (PAYLOAD_ENCODER == 3)

#define LPP_GPS_CHANNEL 20
#define LPP_COUNT_WIFI_CHANNEL 21
#define LPP_COUNT_BLE_CHANNEL 22
#define LPP_BATT_CHANNEL 23
#define LPP_BUTTON_CHANNEL 24
#define LPP_ADR_CHANNEL 25
#define LPP_TEMPERATURE_CHANNEL 26
#define LPP_ALARM_CHANNEL 27
#define LPP_MSG_CHANNEL 28
#define LPP_HUMIDITY_CHANNEL 29
#define LPP_BAROMETER_CHANNEL 30
#define LPP_AIR_CHANNEL 31 

#endif

// MyDevices CayenneLPP types
#define LPP_GPS 136          // 3 byte lon/lat 0.0001 °, 3 bytes alt 0.01m
#define LPP_TEMPERATURE 103  // 2 bytes, 0.1°C signed MSB
#define LPP_DIGITAL_INPUT 0  // 1 byte
#define LPP_DIGITAL_OUTPUT 1 // 1 byte
#define LPP_ANALOG_INPUT 2   // 2 bytes, 0.01 signed
#define LPP_LUMINOSITY 101   // 2 bytes, 1 lux unsigned
#define LPP_PRESENCE 102     //	1 byte
#define LPP_HUMIDITY 104     // 1 byte, 0.5 % unsigned
#define LPP_BAROMETER 115    // 2 bytes, hPa unsigned MSB

class PayloadConvert {

public:
  PayloadConvert(uint8_t size);
  ~PayloadConvert();

  void reset(void);
  uint8_t getSize(void);
  uint8_t *getBuffer(void);
  void addCount(uint16_t value1, uint16_t value2);
  void addConfig(configData_t value);
  void addStatus(uint16_t voltage, uint64_t uptime, float cputemp, uint32_t mem,
                 uint8_t reset1, uint8_t reset2);
  void addAlarm(int8_t rssi, uint8_t message);
  void addGPS(gpsStatus_t value);
  void addBME(bmeStatus_t value);
  void addButton(uint8_t value);
  void addSensor(uint8_t[]);

#if PAYLOAD_ENCODER == 1 // format plain

private:
  uint8_t *buffer;
  uint8_t cursor;

#elif PAYLOAD_ENCODER == 2 // format packed

private:
  uint8_t *buffer;
  uint8_t cursor;
  void intToBytes(uint8_t pos, int32_t i, uint8_t byteSize);
  void writeUptime(uint64_t unixtime);
  void writeLatLng(double latitude, double longitude);
  void writeUint32(uint32_t i);
  void writeUint16(uint16_t i);
  void writeUint8(uint8_t i);
  void writeFloat(float value);
  void writeUFloat(float value);
  void writePressure(float value);
  void writeVersion(char * version);
  void writeBitmap(bool a, bool b, bool c, bool d, bool e, bool f, bool g,
                   bool h);

#elif (PAYLOAD_ENCODER == 3 || PAYLOAD_ENCODER == 4) // format cayenne lpp

private:
  uint8_t *buffer;
  uint8_t maxsize;
  uint8_t cursor;

#else
#error "No valid payload converter defined"
#endif
};

extern PayloadConvert payload;

#endif // _PAYLOAD_H_