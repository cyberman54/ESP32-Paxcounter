#if (HAS_GPS)

#include "globals.h"
#include "gpsread.h"

// Local logging tag
static const char TAG[] = __FILE__;

TinyGPSPlus gps;
TaskHandle_t GpsTask;
HardwareSerial GPS_Serial(1); // use UART #1

// helper functions to send UBX commands to ublox gps chip

void sendPacket(byte *packet, byte len) {

  uint8_t CK_A = 0;
  uint8_t CK_B = 0;

  for (int i = 0; i < len; i++)
    GPS_Serial.write(packet[i]);

  // calculate and send Fletcher checksum
  for (int i = 2; i < len; i++) {
    CK_A += packet[i];
    CK_B += CK_A;
  }
  GPS_Serial.write(CK_A);
  GPS_Serial.write(CK_B);
}

void restoreDefaults() {
  // UBX CFG-CFG packet
  byte packet[] = {
      0xB5,       // sync char 1
      0x62,       // sync char 2
      0x06,       // class
      0x09,       // id
      0x0D,       // length
      0x00,       // .
      0b00011111, // clearmask
      0b00000110, // .
      0x00,       // .
      0x00,       // .
      0x00,       // savemask
      0x00,       // .
      0x00,       // .
      0x00,       // .
      0b00011111, // loadmask
      0b00000110, // .
      0x00,       // .
      0x00,       // .
      0b00010001  // devicemask
  };

  sendPacket(packet, sizeof(packet));
}

void setTimePulse() {
  // UBX TIM-TP packet
  byte packet[] = {
      0xB5,       // sync char 1
      0x62,       // sync char 2
      0x06,       // class
      0x07,       // id
      0x14,       // length
      0x40,       // time interval for time pulse [us]
      0x42,       // -> 1 sec = 1000000us
      0x0F,       // .
      0x00,       // .
      0xE8,       // length of time pulse [us]
      0x03,       // -> 1000us
      0x00,       // .
      0x00,       // .
      0x01,       // status -> positive edge
      0x00,       // timeRef -> UTC
      0b00000001, // syncMode asynchronized
      0x00,       // reserved
      0x00,       // antenna cable delay [ns]
      0x00,       // .
      0x00,       // receiver rf group delay [ns]
      0x00,       // .
      0x00,       // user time function delay [ns]
      0x00,       // .
      0x00,       // .
      0x00        // .
  };

  sendPacket(packet, sizeof(packet));
}

void disableNmea() {

  // for tinygps++ we need only $GPGGA and $GPRMC
  // thus, we disable all other NMEA messages

  // Array of two bytes for CFG-MSG packets payload.
  byte messages[][2] = {{0xF0, 0x01}, {0xF0, 0x02}, {0xF0, 0x03}, {0xF0, 0x05},
                        {0xF0, 0x06}, {0xF0, 0x07}, {0xF0, 0x08}, {0xF0, 0x09},
                        {0xF0, 0x0A}, {0xF0, 0x0E}, {0xF1, 0x00}, {0xF1, 0x03},
                        {0xF1, 0x04}, {0xF1, 0x05}, {0xF1, 0x06}};

  // UBX CFG-MSG packet
  byte packet[] = {
      0xB5, // sync char 1
      0x62, // sync char 2
      0x06, // class
      0x01, // id
      0x03, // length
      0x00, // .
      0x00, // payload (first byte from messages array element)
      0x00, // payload (second byte from messages array element)
      0x00  // payload (zero to disable message)
  };

  byte packetSize = sizeof(packet);

  // Offset to the place where payload starts.
  byte payloadOffset = 6;

  // Iterate over the messages array.
  for (byte i = 0; i < sizeof(messages) / sizeof(*messages); i++) {
    // Copy two bytes of payload to the packet buffer.
    for (byte j = 0; j < sizeof(*messages); j++) {
      packet[payloadOffset + j] = messages[i][j];
    }
    sendPacket(packet, packetSize);
  }
}

void changeBaudrate(uint32_t baudRate) {
  // UBX CFG-PRT packet
  byte packet[] = {
      0xB5,                   // sync char 1
      0x62,                   // sync char 2
      0x06,                   // class
      0x00,                   // id
      0x14,                   // length
      0x00,                   // .
      0x01,                   // portID (UART 1)
      0x00,                   // reserved
      0x00,                   // txReady
      0x00,                   // .
      0b11010000,             // UART mode: 8N1
      0b00001000,             // .
      0x00,                   // .
      0x00,                   // .
      (byte)baudRate,         // baudrate
      (byte)(baudRate >> 8),  // .
      (byte)(baudRate >> 16), // .
      (byte)(baudRate >> 24), // .
      0b00000011,             // input protocols: NMEA + UBX
      0b00000000,             // .
      0b00000010,             // output protocols: NMEA
      0x00000000,             // .
      0x00,                   // reserved
      0x00,                   // .
      0x00,                   // .
      0x00                    // .
  };

  sendPacket(packet, sizeof(packet));
}

// initialize and configure GPS
int gps_init(void) {

  ESP_LOGI(TAG, "Opening serial GPS");

  GPS_Serial.begin(GPS_SERIAL);

  restoreDefaults();
  delay(100);

  changeBaudrate(GPS_BAUDRATE);
  delay(100);
  GPS_Serial.flush();
  GPS_Serial.updateBaudRate(GPS_BAUDRATE);

  disableNmea();
  setTimePulse();

  return 1;

} // gps_init()

// store current GPS location data in struct
void gps_storelocation(gpsStatus_t *gps_store) {
  if (gps.location.isUpdated() && gps.location.isValid() &&
      (gps.location.age() < 1500)) {
    gps_store->latitude = (int32_t)(gps.location.lat() * 1e6);
    gps_store->longitude = (int32_t)(gps.location.lng() * 1e6);
    gps_store->satellites = (uint8_t)gps.satellites.value();
    gps_store->hdop = (uint16_t)gps.hdop.value();
    gps_store->altitude = (int16_t)gps.altitude.meters();
  }
}

bool gps_hasfix() {
  // adapted from source:
  // https://github.com/hottimuc/Lora-TTNMapper-T-Beam/blob/master/fromV08/gps.cpp
  return (gps.location.isValid() && gps.location.age() < 4000 &&
          gps.hdop.isValid() && gps.hdop.value() <= 600 &&
          gps.hdop.age() < 4000 && gps.altitude.isValid() &&
          gps.altitude.age() < 4000);
}

// function to poll UTC time from GPS NMEA data; note: this is costly
time_t get_gpstime(uint16_t *msec) {

  *msec = 0;

  // did we get a current date & time?
  if (gps.time.isValid() && gps.date.isValid() && gps.time.age() < 1000) {

    // convert tinygps time format to struct tm format
    struct tm gps_tm = {0};
    gps_tm.tm_sec = gps.time.second();
    gps_tm.tm_min = gps.time.minute();
    gps_tm.tm_hour = gps.time.hour();
    gps_tm.tm_mday = gps.date.day();
    gps_tm.tm_mon = gps.date.month() - 1;    // 1-12 -> 0-11
    gps_tm.tm_year = gps.date.year() - 1900; // 2000+ -> years since 1900

    // convert UTC tm to time_t epoch
    gps_tm.tm_isdst = 0; // UTC has no DST
    time_t t = mkgmtime(&gps_tm);

#ifdef GPS_INT
    // if we have a recent GPS PPS pulse, sync on top of next second
    if (millis() - lastPPS < 1000)
      *msec = (uint16_t)(millis() - lastPPS);
    else {
      ESP_LOGD(TAG, "no PPS from GPS");
      return 0;
    }
#endif

    return t;
  }

  ESP_LOGD(TAG, "no valid GPS time");
  return 0;

} // get_gpstime()

// GPS serial feed FreeRTos Task
void gps_loop(void *pvParameters) {

  _ASSERT((uint32_t)pvParameters == 1); // FreeRTOS check

  while (1) {

    while (cfg.payloadmask & GPS_DATA) {
      // feed GPS decoder with serial NMEA data from GPS device
      while (GPS_Serial.available()) {
        if (gps.encode(GPS_Serial.read())) {

          // show NMEA data, very noisy, for debugging GPS
          // ESP_LOGV(
          //    TAG,
          //    "GPS NMEA data: chars %u / passed %u / failed: %u / with fix:
          //    %u", gps.charsProcessed(), gps.passedChecksum(),
          //    gps.failedChecksum(), gps.sentencesWithFix());

          delay(5); // yield after each sentence to crack NMEA burst
        }
      } // read from serial buffer loop
      delay(5);
    }

    delay(1000);
  } // infinite while loop

} // gps_loop()

#endif // HAS_GPS
