// SDS011 dust sensor PM2.5 and PM10
// ---------------------
//
// By R. Zschiegner (rz@madavi.de)
// April 2016
//
// Documentation:
//    - The iNovaFitness SDS011 datasheet
//
// modified by AQ - 2018-11-18
//

#include "SDS011.h"

static const byte SDS_SLEEP[] = {
  0xAA, // head
  0xB4, // command id
  0x06, // data byte 1
  0x01, // data byte 2 (set mode)
  0x00, // data byte 3 (sleep)
  0x00, // data byte 4
  0x00, // data byte 5
  0x00, // data byte 6
  0x00, // data byte 7
  0x00, // data byte 8
  0x00, // data byte 9
  0x00, // data byte 10
  0x00, // data byte 11
  0x00, // data byte 12
  0x00, // data byte 13
  0xFF, // data byte 14 (device id byte 1)
  0xFF, // data byte 15 (device id byte 2)
  0x05, // checksum
  0xAB  // tail
};

static const byte SDS_START[] = {
  0xAA, 0xB4, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x06, 0xAB};

static const byte SDS_CONT_MODE[] = {
  0xAA, 0xB4, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x07, 0xAB};
  
static const byte SDS_VERSION[] = {
  0xAA, 0xB4, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x05, 0xAB};

const uint8_t SDS_cmd_len = 19;

SDS011::SDS011(void) {

}

// --------------------------------------------------------
// SDS011:read
// --------------------------------------------------------
int SDS011::read(float *p25, float *p10) {
  byte buffer;
  int value;
  int len = 0;
  int pm10_serial = 0;
  int pm25_serial = 0;
  int checksum_is;
  int checksum_ok = 0;
  int error = 1;
  
  while ((sds_data->available() > 0) && (sds_data->available() >= (10-len))) {
    buffer = sds_data->read();
    value = int(buffer);
    switch (len) {
      case (0): if (value != 170) { len = -1; }; break;
      case (1): if (value != 192) { len = -1; }; break;
      case (2): pm25_serial = value; checksum_is = value; break;
      case (3): pm25_serial += (value << 8); checksum_is += value; break;
      case (4): pm10_serial = value; checksum_is += value; break;
      case (5): pm10_serial += (value << 8); checksum_is += value; break;
      case (6): checksum_is += value; break;
      case (7): checksum_is += value; break;
      case (8): if (value == (checksum_is % 256)) { checksum_ok = 1; } else { len = -1; }; break;
      case (9): if (value != 171) { len = -1; }; break;
    }
    len++;
    if (len == 10 && checksum_ok == 1) {
      *p10 = (float)pm10_serial/10.0;
      *p25 = (float)pm25_serial/10.0;
      len = 0; checksum_ok = 0; pm10_serial = 0.0; pm25_serial = 0.0; checksum_is = 0;
      error = 0;
    }
    yield();
  }
  return error;
}

// --------------------------------------------------------
// SDS011:sleep
// --------------------------------------------------------
void SDS011::sleep() {
    SDS_cmd(SDS_STOP_CMD);
}

// --------------------------------------------------------
// SDS011:wakeup
// --------------------------------------------------------
void SDS011::wakeup() {
    SDS_cmd(SDS_START_CMD);
}

// --------------------------------------------------------
// SDS011:continous mode
// --------------------------------------------------------
void SDS011::contmode(int noOfMinutes)
{
     byte buffer[SDS_cmd_len];
     memcpy(buffer, SDS_CONT_MODE, SDS_cmd_len);
     buffer[4] = (byte) noOfMinutes;
     buffer[17] = calcChecksum( buffer );
    for (uint8_t i = 0; i < SDS_cmd_len; i++) {
         sds_data->write(buffer[i]);
    }
    sds_data->flush();
    while (sds_data->available() > 0) {
        sds_data->read();
    }    
//     SDS_cmd(SDS_CONTINUOUS_MODE_CMD); 
}

/*****************************************************************
 * send SDS011 command (start, stop, continuous mode, version    *
 *****************************************************************/
void SDS011::SDS_cmd(const uint8_t cmd) 
{
    byte buf[SDS_cmd_len];
    switch (cmd) {
    case SDS_START_CMD:
      memcpy(buf, SDS_START, SDS_cmd_len);
      break;
    case SDS_STOP_CMD:
      memcpy(buf, SDS_SLEEP, SDS_cmd_len);
      break;
    case SDS_CONTINUOUS_MODE_CMD:
      memcpy(buf, SDS_CONT_MODE, SDS_cmd_len);
      break;
    case SDS_VERSION_DATE_CMD:
      memcpy(buf, SDS_VERSION, SDS_cmd_len);
      break;
    default:
        return;
    }
    for (uint8_t i = 0; i < SDS_cmd_len; i++) {
         sds_data->write(buf[i]);
    }
    sds_data->flush();
    while (sds_data->available() > 0) {
        sds_data->read();
    }     

}

// --------------------------------------------------------
// SDS011: calculate checksum
// --------------------------------------------------------
uint8_t SDS011::calcChecksum( byte *buffer )
{
  uint8_t value = 0;

  for (uint8_t i = 2; i < 17; i++ )
  {
      value += buffer[i];
      value &= 0xff;
  }
  return value;
}

void SDS011::begin(uint8_t pin_rx, uint8_t pin_tx) {
  _pin_rx = pin_rx;
  _pin_tx = pin_tx;

  SoftwareSerial *softSerial = new SoftwareSerial(_pin_rx, _pin_tx);
  softSerial->begin(9600);

  sds_data = softSerial;
}

void SDS011::begin(HardwareSerial* serial) {
  Serial.println("SDS011::begin");
//  serial->begin(9600);    // why do I have to remove this line?
  sds_data = serial;
}

void SDS011::begin(SoftwareSerial* serial) {
  serial->begin(9600);
  sds_data = serial;
}
