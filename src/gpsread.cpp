#if (HAS_GPS)

#include "globals.h"
#include "gpsread.h"

// Local logging tag
static const char TAG[] = __FILE__;

// we use NMEA ZDA sentence field 1 for time synchronization
// ZDA gives time for preceding pps pulse
// downsight is that it does not have a constant offset
// thus precision is only +/- 1 second

TinyGPSPlus gps;
TinyGPSCustom gpstime(gps, "GPZDA", 1);  // field 1 = UTC time (hhmmss.ss)
TinyGPSCustom gpsday(gps, "GPZDA", 2);   // field 2 = day (01..31)
TinyGPSCustom gpsmonth(gps, "GPZDA", 3); // field 3 = month (01..12)
TinyGPSCustom gpsyear(gps, "GPZDA", 4);  // field 4 = year (4-digit)
static const String ZDA_Request = "$EIGPQ,ZDA*39\r\n";
TaskHandle_t GpsTask;

HardwareSerial GPS_Serial(1); // use UART #1
static uint16_t nmea_txDelay_ms =
    (tx_Ticks(NMEA_FRAME_SIZE, GPS_SERIAL) / portTICK_PERIOD_MS);

// helper functions to send UBX commands to ublox gps chip

// Send the packet specified to the receiver.
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

// Send a packet to the receiver to restore default configuration.
void restoreDefaults() {
  // CFG-CFG packet.
  byte packet[] = {
      0xB5,       // sync char 1
      0x62,       // sync char 2
      0x06,       // class
      0x09,       // id
      0x0D,       // length
      0x00,       // length
      0b00011111, // clearmask
      0b00000110, // clearmask
      0x00,       // clearmask
      0x00,       // clearmask
      0x00,       // savemask
      0x00,       // savemask
      0x00,       // savemask
      0x00,       // savemask
      0b00011111, // loadmask
      0b00000110, // loadmask
      0x00,       // loadmask
      0x00,       // loadmask
      0b00010001  // devicemask
  };

  sendPacket(packet, sizeof(packet));
}

// Send a set of packets to the receiver to disable NMEA messages.
void disableNmea() {

  // for tinygps++ we need only $GPGGA and $GPRMC
  // for time we use $GPZDA
  // we disable all others

  // Array of two bytes for CFG-MSG packets payload.
  byte messages[][2] = {{0xF0, 0x01}, {0xF0, 0x02}, {0xF0, 0x03}, {0xF0, 0x05},
                        {0xF0, 0x06}, {0xF0, 0x07}, {0xF0, 0x09}, {0xF0, 0x0A},
                        {0xF0, 0x0E}, {0xF1, 0x00}, {0xF1, 0x03}, {0xF1, 0x04},
                        {0xF1, 0x05}, {0xF1, 0x06}};

  // CFG-MSG packet buffer.
  byte packet[] = {
      0xB5, // sync char 1
      0x62, // sync char 2
      0x06, // class
      0x01, // id
      0x03, // length
      0x00, // length
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

// Send a packet to the receiver to change baudrate to 115200.
void changeBaudrate(uint32_t baudRate) {
  // CFG-PRT packet.
  byte packet[] = {
      0xB5,                   // sync char 1
      0x62,                   // sync char 2
      0x06,                   // class
      0x00,                   // id
      0x14,                   // length
      0x00,                   // length
      0x01,                   // portID (UART 1)
      0x00,                   // reserved
      0x00,                   // reserved
      0x00,                   // reserved
      0b11010000,             // UART mode: 8bit
      0b00001000,             // UART mode: No Parity, 1 Stopbit
      0x00,                   // UART mode
      0x00,                   // UART mode
      (byte)baudRate,         // baudrate (4 bytes)
      (byte)(baudRate >> 4),  // .
      (byte)(baudRate >> 8),  // .
      (byte)(baudRate >> 12), // .
      0b00000011,             // input protocols: NMEA + UBX
      0b00000000,             // input protocols
      0b00000010,             // output protocols: NMEA
      0x00000000,             // output protocols
      0x00,                   // reserved
      0x00,                   // reserved
      0x00,                   // reserved
      0x00                    // reserved
  };

  sendPacket(packet, sizeof(packet));
}

// Send a packet to the receiver to change frequency to 100 ms.
void changeFrequency() {
  // CFG-RATE packet.
  byte packet[] = {
      0xB5, // sync char 1
      0x62, // sync char 2
      0x06, // class
      0x08, // id
      0x06, // length
      0x00, // length
      0x64, // Measurement rate 100ms
      0x00, // Measurement rate
      0x01, // Measurement cycles
      0x00, // Measurement cycles
      0x01, // Alignment to reference time: GPS time
      0x00  // payload
  };

  sendPacket(packet, sizeof(packet));
}

// initialize and configure GPS
int gps_init(void) {

  restoreDefaults();

  ESP_LOGI(TAG, "Opening serial GPS");
  GPS_Serial.begin(GPS_SERIAL);
  changeBaudrate(GPS_BAUDRATE);
  delay(100);
  GPS_Serial.flush();
  GPS_Serial.updateBaudRate(GPS_BAUDRATE);

  disableNmea();
  changeFrequency();
  // enableNavTimeUTC();

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

  // poll NMEA ZDA sentence
  GPS_Serial.print(ZDA_Request);
  // wait for gps NMEA answer
  // vTaskDelay(tx_Ticks(NMEA_FRAME_SIZE, GPS_SERIAL));

  // did we get a current date & time?
  if (gpstime.isValid()) {

    uint32_t delay_ms =
        gpstime.age() + nmea_txDelay_ms + NMEA_COMPENSATION_FACTOR;
    uint32_t zdatime = atof(gpstime.value());

    // convert UTC time from gps NMEA ZDA sentence to tm format
    struct tm gps_tm = {0};
    gps_tm.tm_sec = zdatime % 100;                 // second (UTC)
    gps_tm.tm_min = (zdatime / 100) % 100;         // minute (UTC)
    gps_tm.tm_hour = zdatime / 10000;              // hour (UTC)
    gps_tm.tm_mday = atoi(gpsday.value());         // day, 01 to 31
    gps_tm.tm_mon = atoi(gpsmonth.value()) - 1;    // month, 01 to 12
    gps_tm.tm_year = atoi(gpsyear.value()) - 1900; // year, YYYY

    // convert UTC tm to time_t epoch
    gps_tm.tm_isdst = 0; // UTC has no DST
    time_t t = mkgmtime(&gps_tm);

    // add protocol delay with millisecond precision
    t += (time_t)(delay_ms / 1000);
    *msec = delay_ms % 1000; // fractional seconds

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
      while (GPS_Serial.available())
        if (gps.encode(GPS_Serial.read()))
          break; // NMEA sentence complete

      // (only) while device time is not set or unsynched, and we have a valid
      // GPS time, we call calibrateTime to poll time immeditately from GPS
      if ((timeSource == _unsynced || timeSource == _set) &&
          (gpstime.isUpdated() && gpstime.isValid() && gpstime.age() < 1000))
        calibrateTime();

      // show NMEA data, very noisy,  useful only for debugging GPS
      // ESP_LOGV(TAG, "GPS NMEA data: passed %u / failed: %u / with fix:
      //                  %u", gps.passedChecksum(), gps.failedChecksum(), gps
      //                       .sentencesWithFix());

      delay(2);
    } // inner while loop

    delay(1000);
  } // outer while loop

} // gps_loop()

#endif // HAS_GPS
