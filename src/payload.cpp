
#include "globals.h"
#include "payload.h"

/* ---------------- plain format without special encoding ---------- */

TTNplain::TTNplain(uint8_t size) {
  buffer = (uint8_t *)malloc(size);
  cursor = 0;
}

TTNplain::~TTNplain(void) { free(buffer); }

void TTNplain::reset(void) { cursor = 0; }

uint8_t TTNplain::getSize(void) { return cursor; }

uint8_t *TTNplain::getBuffer(void) { return buffer; }

void TTNplain::addCount(uint16_t value1, uint16_t value2) {
  buffer[cursor++] = value1 >> 8;
  buffer[cursor++] = value1;
  buffer[cursor++] = value2 >> 8;
  buffer[cursor++] = value2;
}

void TTNplain::addGPS(gpsStatus_t value) {
  buffer[cursor++] = value.latitude >> 24;
  buffer[cursor++] = value.latitude >> 16;
  buffer[cursor++] = value.latitude >> 8;
  buffer[cursor++] = value.latitude;
  buffer[cursor++] = value.longitude >> 24;
  buffer[cursor++] = value.longitude >> 16;
  buffer[cursor++] = value.longitude >> 8;
  buffer[cursor++] = value.longitude;
  buffer[cursor++] = value.satellites;
  buffer[cursor++] = value.hdop >> 8;
  buffer[cursor++] = value.hdop;
  buffer[cursor++] = value.altitude >> 8;
  buffer[cursor++] = value.altitude;
}

void TTNplain::addConfig(configData_t value) {
  buffer[cursor++] = value.lorasf;
  buffer[cursor++] = value.adrmode;
  buffer[cursor++] = value.screensaver;
  buffer[cursor++] = value.screenon;
  buffer[cursor++] = value.countermode;
  buffer[cursor++] = value.rssilimit >> 8;
  buffer[cursor++] = value.rssilimit;
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

void TTNplain::addStatus(uint16_t voltage, uint64_t uptime, float cputemp) {
  buffer[cursor++] = voltage >> 8;
  buffer[cursor++] = voltage;
  buffer[cursor++] = uptime >> 56;
  buffer[cursor++] = uptime >> 48;
  buffer[cursor++] = uptime >> 40;
  buffer[cursor++] = uptime >> 32;
  buffer[cursor++] = uptime >> 24;
  buffer[cursor++] = uptime >> 16;
  buffer[cursor++] = uptime >> 8;
  buffer[cursor++] = uptime;
  buffer[cursor++] = (uint32_t)cputemp >> 24;
  buffer[cursor++] = (uint32_t)cputemp >> 16;
  buffer[cursor++] = (uint32_t)cputemp >> 8;
  buffer[cursor++] = (uint32_t)cputemp;
}

/* ---------------- packed format with LoRa serialization Encoder ---------- */

TTNserialized::TTNserialized(uint8_t size) { buffer = (uint8_t *)malloc(size); }

TTNserialized::~TTNserialized(void) { free(buffer); }

void TTNserialized::reset(void) { } // buggy! to be done, we need to clear the buffer here, but how? 

uint8_t TTNserialized::getSize(void) { return sizeof(buffer); }

uint8_t *TTNserialized::getBuffer(void) { return buffer; }

void TTNserialized::addCount(uint16_t value1, uint16_t value2) {
  LoraEncoder message(buffer);
  message.writeUint16(value1);
  message.writeUint16(value2);
}

void TTNserialized::addGPS(gpsStatus_t value) {
  LoraEncoder message(buffer);
  message.writeLatLng(value.latitude, value.longitude);
  message.writeUint8(value.satellites);
  message.writeUint16(value.hdop);
  message.writeUint16(value.altitude);
}

void TTNserialized::addConfig(configData_t value) {
  LoraEncoder message(buffer);
  message.writeUint8(value.lorasf);
  message.writeUint16(value.rssilimit);
  message.writeUint8(value.sendcycle);
  message.writeUint8(value.wifichancycle);
  message.writeUint8(value.blescantime);
  message.writeUint8(value.rgblum);
  message.writeBitmap(
      value.adrmode ? true : false, value.screensaver ? true : false,
      value.screenon ? true : false, value.countermode ? true : false,
      value.blescan ? true : false, value.wifiant ? true : false,
      value.vendorfilter ? true : false, value.gpsmode ? true : false);
}

void TTNserialized::addStatus(uint16_t voltage, uint64_t uptime,
                              float cputemp) {
  LoraEncoder message(buffer);
  message.writeUint16(voltage);
  message.writeUnixtime(uptime);
  message.writeTemperature(cputemp);
}

/* ---------------- Cayenne LPP format ---------- */

CayenneLPP::CayenneLPP(uint8_t size) {
  buffer = (uint8_t *)malloc(size);
  cursor = 0;
}

CayenneLPP::~CayenneLPP(void) { free(buffer); }

void CayenneLPP::reset(void) { cursor = 0; }

uint8_t CayenneLPP::getSize(void) { return cursor; }

uint8_t *CayenneLPP::getBuffer(void) { return buffer; }

void CayenneLPP::addCount(uint16_t value1, uint16_t value2) {
  buffer[cursor++] = LPP_COUNT_WIFI_CHANNEL;
  buffer[cursor++] = LPP_LUMINOSITY; // workaround, type meter not found?
  buffer[cursor++] = value1 >> 8;
  buffer[cursor++] = value1;
  buffer[cursor++] = LPP_COUNT_BLE_CHANNEL;
  buffer[cursor++] = LPP_LUMINOSITY; // workaround, type meter not found?
  buffer[cursor++] = value2 >> 8;
  buffer[cursor++] = value2;
}

void CayenneLPP::addGPS(gpsStatus_t value) {
  int32_t lat = value.latitude / 100;
  int32_t lon = value.longitude / 100;
  int32_t alt = value.altitude;
  buffer[cursor++] = LPP_GPS_CHANNEL;
  buffer[cursor++] = LPP_GPS;
  buffer[cursor++] = lat >> 16;
  buffer[cursor++] = lat >> 8;
  buffer[cursor++] = lat;
  buffer[cursor++] = lon >> 16;
  buffer[cursor++] = lon >> 8;
  buffer[cursor++] = lon;
  buffer[cursor++] = alt >> 16;
  buffer[cursor++] = alt >> 8;
  buffer[cursor++] = alt;
}

void CayenneLPP::addConfig(configData_t value) {
  buffer[cursor++] = LPP_ADR_CHANNEL;
  buffer[cursor++] = LPP_DIGITAL_INPUT;
  buffer[cursor++] = value.adrmode;
}

void CayenneLPP::addStatus(uint16_t voltage, uint64_t uptime, float cputemp) {
  buffer[cursor++] = LPP_BATT_CHANNEL;
  buffer[cursor++] = LPP_ANALOG_INPUT;
  buffer[cursor++] = voltage >> 8;
  buffer[cursor++] = voltage;
  buffer[cursor++] = LPP_TEMP_CHANNEL;
  buffer[cursor++] = LPP_TEMPERATURE;
  buffer[cursor++] = (uint16_t)cputemp >> 8;
  buffer[cursor++] = (uint16_t)cputemp;
}