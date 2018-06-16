
#include "globals.h"
#include "payload.h"

TTNplain::TTNplain(uint8_t size) : maxsize(size) {
  buffer = (uint8_t *)malloc(size);
  cursor = 0;
}

TTNplain::~TTNplain(void) { free(buffer); }

void TTNplain::reset(void) { cursor = 0; }

uint8_t TTNplain::getSize(void) { return cursor; }

uint8_t *TTNplain::getBuffer(void) {
  //    uint8_t[cursor] result;
  //    memcpy(result, buffer, cursor);
  //    return result;
  return buffer;
}

uint8_t TTNplain::copy(uint8_t *dst) {
  memcpy(dst, buffer, cursor);
  return cursor;
}

void TTNplain::addCount(uint16_t value1, uint16_t value2) {
  cursor = TTN_PAYLOAD_COUNTER;
  buffer[cursor] = (macs_wifi & 0xff00) >> 8;
  buffer[cursor++] = macs_wifi & 0xff;
  buffer[cursor++] = (macs_ble & 0xff00) >> 8;
  buffer[cursor++] = macs_ble & 0xff;
}

void TTNplain::addGPS(gpsStatus_t value) {
  cursor = TTN_PAYLOAD_GPS;
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
  cursor = TTN_PAYLOAD_CONFIG;
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
}

void TTNplain::addStatus(uint16_t voltage, uint64_t uptime, float cputemp) {
  cursor = TTN_PAYLOAD_STATUS;
  buffer[cursor++] = voltage >> 8;
  buffer[cursor++] = voltage;
  buffer[cursor++] = uptime >> 56;
  buffer[cursor++] = uptime >> 48;
  buffer[cursor++] = uptime >> 40;
  buffer[cursor++] = uptime >> 32;
  buffer[cursor++] = uptime >> 24;
  buffer[cursor++] = uptime >> 32;
  buffer[cursor++] = uptime >> 16;
  buffer[cursor++] = uptime >> 8;
  buffer[cursor++] = (uint32_t) cputemp >> 24;
  buffer[cursor++] = (uint32_t) cputemp >> 16;
  buffer[cursor++] = (uint32_t) cputemp >> 8;
  buffer[cursor++] = (uint32_t) cputemp;
}