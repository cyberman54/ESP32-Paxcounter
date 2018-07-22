
#include "globals.h"
#include "payload.h"

PayloadConvert::PayloadConvert(uint8_t size) {
  buffer = (uint8_t *)malloc(size);
  cursor = 0;
}

PayloadConvert::~PayloadConvert(void) { free(buffer); }

void PayloadConvert::reset(void) { cursor = 0; }

uint8_t PayloadConvert::getSize(void) { return cursor; }

uint8_t *PayloadConvert::getBuffer(void) { return buffer; }

/* ---------------- plain format without special encoding ---------- */

#if PAYLOAD_ENCODER == 1

void PayloadConvert::addCount(uint16_t value1, uint16_t value2) {
  buffer[cursor++] = highByte(value1);
  buffer[cursor++] = lowByte(value1);
  buffer[cursor++] = highByte(value2);
  buffer[cursor++] = lowByte(value2);
}

void PayloadConvert::addConfig(configData_t value) {
  buffer[cursor++] = value.lorasf;
  buffer[cursor++] = value.txpower;
  buffer[cursor++] = value.adrmode;
  buffer[cursor++] = value.screensaver;
  buffer[cursor++] = value.screenon;
  buffer[cursor++] = value.countermode;
  buffer[cursor++] = highByte(value.rssilimit);
  buffer[cursor++] = lowByte(value.rssilimit);
  buffer[cursor++] = value.sendcycle;
  buffer[cursor++] = value.wifichancycle;
  buffer[cursor++] = value.blescantime;
  buffer[cursor++] = value.blescan;
  buffer[cursor++] = value.wifiant;
  buffer[cursor++] = value.vendorfilter;
  buffer[cursor++] = value.rgblum;
  buffer[cursor++] = value.gpsmode;
  memcpy(buffer + cursor, value.version, 10);
  cursor += 10;
}

void PayloadConvert::addStatus(uint16_t voltage, uint64_t uptime,
                               float cputemp) {
  uint32_t temp = (uint32_t)cputemp;
  buffer[cursor++] = highByte(voltage);
  buffer[cursor++] = lowByte(voltage);
  buffer[cursor++] = (byte) ((uptime & 0xFF00000000000000) >> 56 );
  buffer[cursor++] = (byte) ((uptime & 0x00FF000000000000) >> 48 );
  buffer[cursor++] = (byte) ((uptime & 0x0000FF0000000000) >> 40 );
  buffer[cursor++] = (byte) ((uptime & 0x000000FF00000000) >> 32 );
  buffer[cursor++] = (byte) ((uptime & 0x00000000FF000000) >> 24 );
  buffer[cursor++] = (byte) ((uptime & 0x0000000000FF0000) >> 16 );
  buffer[cursor++] = (byte) ((uptime & 0x000000000000FF00) >> 8 );
  buffer[cursor++] = (byte) ((uptime & 0x00000000000000FF) );
  buffer[cursor++] = (byte) ((temp & 0xFF000000) >> 24 );
  buffer[cursor++] = (byte) ((temp & 0x00FF0000) >> 16 );
  buffer[cursor++] = (byte) ((temp & 0x0000FF00) >> 8 );
  buffer[cursor++] = (byte) ((temp & 0x000000FF) );
}

#ifdef HAS_GPS
void PayloadConvert::addGPS(gpsStatus_t value) {
  buffer[cursor++] = (byte) ((value.latitude & 0xFF000000) >> 24 );
  buffer[cursor++] = (byte) ((value.latitude & 0x00FF0000) >> 16 );
  buffer[cursor++] = (byte) ((value.latitude & 0x0000FF00) >> 8 );
  buffer[cursor++] = (byte) ((value.latitude & 0x000000FF) );
  buffer[cursor++] = (byte) ((value.longitude & 0xFF000000) >> 24 );
  buffer[cursor++] = (byte) ((value.longitude & 0x00FF0000) >> 16 );
  buffer[cursor++] = (byte) ((value.longitude & 0x0000FF00) >> 8 );
  buffer[cursor++] = (byte) ((value.longitude & 0x000000FF) );
  buffer[cursor++] = value.satellites;
  buffer[cursor++] = highByte(value.hdop);
  buffer[cursor++] = lowByte(value.hdop);
  buffer[cursor++] = highByte(value.altitude);
  buffer[cursor++] = lowByte(value.altitude);
}
#endif

#ifdef HAS_BUTTON
void PayloadConvert::addButton(uint8_t value) { buffer[cursor++] = value; }
#endif

/* ---------------- packed format with LoRa serialization Encoder ---------- */
// derived from
// https://github.com/thesolarnomad/lora-serialization/blob/master/src/LoraEncoder.cpp

#elif PAYLOAD_ENCODER == 2

void PayloadConvert::addCount(uint16_t value1, uint16_t value2) {
  writeUint16(value1);
  writeUint16(value2);
}

void PayloadConvert::addConfig(configData_t value) {
  writeUint8(value.lorasf);
  writeUint8(value.txpower);
  writeUint16(value.rssilimit);
  writeUint8(value.sendcycle);
  writeUint8(value.wifichancycle);
  writeUint8(value.blescantime);
  writeUint8(value.rgblum);
  writeBitmap(value.adrmode ? true : false, value.screensaver ? true : false,
              value.screenon ? true : false, value.countermode ? true : false,
              value.blescan ? true : false, value.wifiant ? true : false,
              value.vendorfilter ? true : false, value.gpsmode ? true : false);
}

void PayloadConvert::addStatus(uint16_t voltage, uint64_t uptime,
                               float cputemp) {
  writeUint16(voltage);
  writeUptime(uptime);
  writeTemperature(cputemp);
}

#ifdef HAS_GPS
void PayloadConvert::addGPS(gpsStatus_t value) {
  writeLatLng(value.latitude, value.longitude);
  writeUint8(value.satellites);
  writeUint16(value.hdop);
  writeUint16(value.altitude);
}
#endif

#ifdef HAS_BUTTON
void PayloadConvert::addButton(uint8_t value) { writeUint8(value); }
#endif

void PayloadConvert::intToBytes(uint8_t pos, int32_t i, uint8_t byteSize) {
  for (uint8_t x = 0; x < byteSize; x++) {
    buffer[x + pos] = (byte)(i >> (x * 8));
  }
  cursor += byteSize;
}

void PayloadConvert::writeUptime(uint64_t uptime) {
  intToBytes(cursor, uptime, 8);
}

void PayloadConvert::writeLatLng(double latitude, double longitude) {
  intToBytes(cursor, latitude, 4);
  intToBytes(cursor, longitude, 4);
}

void PayloadConvert::writeUint16(uint16_t i) { intToBytes(cursor, i, 2); }

void PayloadConvert::writeUint8(uint8_t i) { intToBytes(cursor, i, 1); }

void PayloadConvert::writeHumidity(float humidity) {
  int16_t h = (int16_t)(humidity * 100);
  intToBytes(cursor, h, 2);
}

/**
 * Uses a 16bit two's complement with two decimals, so the range is
 * -327.68 to +327.67 degrees
 */
void PayloadConvert::writeTemperature(float temperature) {
  int16_t t = (int16_t)(temperature * 100);
  if (temperature < 0) {
    t = ~-t;
    t = t + 1;
  }
  buffer[cursor++] = (byte)((t >> 8) & 0xFF);
  buffer[cursor++] = (byte)t & 0xFF;
}

void PayloadConvert::writeBitmap(bool a, bool b, bool c, bool d, bool e, bool f,
                                 bool g, bool h) {
  uint8_t bitmap = 0;
  // LSB first
  bitmap |= (a & 1) << 7;
  bitmap |= (b & 1) << 6;
  bitmap |= (c & 1) << 5;
  bitmap |= (d & 1) << 4;
  bitmap |= (e & 1) << 3;
  bitmap |= (f & 1) << 2;
  bitmap |= (g & 1) << 1;
  bitmap |= (h & 1) << 0;
  writeUint8(bitmap);
}

/* ---------------- Cayenne LPP format ---------- */
// http://community.mydevices.com/t/cayenne-lpp-2-0/7510

#elif (PAYLOAD_ENCODER == 3 || PAYLOAD_ENCODER == 4)

void PayloadConvert::addCount(uint16_t value1, uint16_t value2) {
  uint16_t val1 = value1 * 100;
  uint16_t val2 = value2 * 100;
#if (PAYLOAD_ENCODER == 3)
  buffer[cursor++] = LPP_COUNT_WIFI_CHANNEL;
#endif
  buffer[cursor++] = LPP_ANALOG_INPUT; // workaround, type meter not found?
  buffer[cursor++] = highByte(val1);
  buffer[cursor++] = lowByte(val1);
#if (PAYLOAD_ENCODER == 3)
  buffer[cursor++] = LPP_COUNT_BLE_CHANNEL;
#endif
  buffer[cursor++] = LPP_ANALOG_INPUT; // workaround, type meter not found?
  buffer[cursor++] = highByte(val2);
  buffer[cursor++] = lowByte(val2);
}

void PayloadConvert::addConfig(configData_t value) {
#if (PAYLOAD_ENCODER == 3)
  buffer[cursor++] = LPP_ADR_CHANNEL;
#endif
  buffer[cursor++] = LPP_DIGITAL_INPUT;
  buffer[cursor++] = value.adrmode;
}

void PayloadConvert::addStatus(uint16_t voltage, uint64_t uptime,
                               float celsius) {
  uint16_t temp = celsius * 10;
  uint16_t volt = voltage / 10;
#if (PAYLOAD_ENCODER == 3)
  buffer[cursor++] = LPP_BATT_CHANNEL;
#endif
  buffer[cursor++] = LPP_ANALOG_INPUT;
  buffer[cursor++] = highByte(volt);
  buffer[cursor++] = lowByte(volt);
#if (PAYLOAD_ENCODER == 3)
  buffer[cursor++] = LPP_TEMP_CHANNEL;
#endif
  buffer[cursor++] = LPP_TEMPERATURE;
  buffer[cursor++] = highByte(temp);
  buffer[cursor++] = lowByte(temp);
}

#ifdef HAS_GPS
void PayloadConvert::addGPS(gpsStatus_t value) {
  int32_t lat = value.latitude / 100;
  int32_t lon = value.longitude / 100;
  int32_t alt = value.altitude * 100;
#if (PAYLOAD_ENCODER == 3)
  buffer[cursor++] = LPP_GPS_CHANNEL;
#endif
  buffer[cursor++] = LPP_GPS;
  buffer[cursor++] = (byte) ((lat & 0xFF0000) >> 16 );
  buffer[cursor++] = (byte) ((lat & 0x00FF00) >> 8 );
  buffer[cursor++] = (byte) ((lat & 0x0000FF) );
  buffer[cursor++] = (byte) ((lon & 0xFF0000) >> 16 );
  buffer[cursor++] = (byte) ((lon & 0x00FF00) >> 8 );
  buffer[cursor++] = (byte) ((lon & 0x0000FF) );
  buffer[cursor++] = (byte) ((alt & 0xFF0000) >> 16 );
  buffer[cursor++] = (byte) ((alt & 0x00FF00) >> 8 );
  buffer[cursor++] = (byte) ((alt & 0x0000FF) );

}
#endif

#ifdef HAS_BUTTON
void PayloadConvert::addButton(uint8_t value) {
#if (PAYLOAD_ENCODER == 3)
  buffer[cursor++] = LPP_BUTTON_CHANNEL;
#endif
  buffer[cursor++] = LPP_DIGITAL_INPUT;
  buffer[cursor++] = value;
}
#endif

#else
#error "No valid payload converter defined"
#endif