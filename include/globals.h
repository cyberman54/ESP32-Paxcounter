#ifndef _GLOBALS_H
#define _GLOBALS_H
#endif

// The mother of all embedded development...
#include <Arduino.h>

// Time functions
#include "microTime.h"
#include <Timezone.h>
#include <RtcDateTime.h>
#include <Ticker.h>

// std::set for unified array functions
#include <set>
#include <array>
#include <algorithm>
#include <bsec.h>

#define _bit(b) (1U << (b))
#define _bitl(b) (1UL << (b))

// bits in payloadmask for filtering payload data
#define COUNT_DATA _bit(0)
#define ALARM_DATA _bit(1)
#define MEMS_DATA _bit(2)
#define GPS_DATA _bit(3)
#define SENSOR1_DATA _bit(4)
#define SENSOR2_DATA _bit(5)
#define SENSOR3_DATA _bit(6)
#define BATT_DATA _bit(7)

// length of display buffer for lmic event messages
#define LMIC_EVENTMSG_LEN 17

// I2C bus access control
#define I2C_MUTEX_LOCK()                                                       \
  (xSemaphoreTake(I2Caccess, pdMS_TO_TICKS(DISPLAYREFRESH_MS)) == pdTRUE)
#define I2C_MUTEX_UNLOCK() (xSemaphoreGive(I2Caccess))

// pseudo system halt function, useful to prevent writeloops to NVRAM
#ifndef _ASSERT
#define _ASSERT(cond)                                                          \
  if ((cond) == 0) {                                                           \
    ESP_LOGE(TAG, "FAILURE in %s:%d", __FILE__, __LINE__);                     \
    mask_user_IRQ();                                                           \
    for (;;)                                                                   \
      ;                                                                        \
  }

#define _seconds() millis() / 1000.0

enum timesource_t { _gps, _rtc, _lora, _set, _unsynced };
enum snifftype_t { MAC_SNIFF_WIFI, MAC_SNIFF_BLE, MAC_SNIFF_BLE_ENS };
enum runmode_t {
  RUNMODE_POWERCYCLE,
  RUNMODE_NORMAL,
  RUNMODE_WAKEUP,
  RUNMODE_UPDATE,
  RUNMODE_SLEEP,
  RUNMODE_MAINTENANCE
};

// Struct holding devices's runtime configuration
// using packed to avoid compiler padding, because struct will be memcpy'd to
// byte array
typedef struct __attribute__((packed)) {
  char version[10];    // Firmware version
  uint8_t loradr;      // 0-15, lora datarate
  uint8_t txpower;     // 2-15, lora tx power
  uint8_t adrmode;     // 0=disabled, 1=enabled
  uint8_t screensaver; // 0=disabled, 1=enabled
  uint8_t screenon;    // 0=disabled, 1=enabled
  uint8_t countermode; // 0=cyclic unconfirmed, 1=cumulative, 2=cyclic confirmed
  int16_t rssilimit;   // threshold for rssilimiter, negative value!
  uint8_t sendcycle;   // payload send cycle [seconds/2]
  uint16_t sleepcycle; // sleep cycle [seconds/10]
  uint8_t wifichancycle; // wifi channel switch cycle [seconds/100]
  uint8_t blescantime;   // BLE scan cycle duration [seconds]
  uint8_t blescan;       // 0=disabled, 1=enabled
  uint8_t wifiscan;      // 0=disabled, 1=enabled
  uint8_t wifiant;       // 0=internal, 1=external (for LoPy/LoPy4)
  uint8_t macfilter;     // 0=disabled, 1=enabled
  uint8_t rgblum;        // RGB Led luminosity (0..100%)
  uint8_t monitormode;   // 0=disabled, 1=enabled
  uint8_t payloadmask;   // bitswitches for payload data
  uint8_t enscount;      // 0=disabled 1= enabled

#ifdef HAS_BME680
  uint8_t
      bsecstate[BSEC_MAX_STATE_BLOB_SIZE + 1]; // BSEC state for BME680 sensor
#endif
} configData_t;

// Struct holding payload for data send queue
typedef struct {
  uint8_t MessageSize;
  uint8_t MessagePort;
  uint8_t Message[PAYLOAD_BUFFER_SIZE];
} MessageBuffer_t;

typedef struct {
  int32_t latitude;
  int32_t longitude;
  uint8_t satellites;
  uint16_t hdop;
  int16_t altitude;
} gpsStatus_t;

typedef struct {
  float iaq;             // IAQ signal
  uint8_t iaq_accuracy;  // accuracy of IAQ signal
  float temperature;     // temperature signal
  float humidity;        // humidity signal
  float pressure;        // pressure signal
  float raw_temperature; // raw temperature signal
  float raw_humidity;    // raw humidity signal
  float gas;             // raw gas sensor signal
} bmeStatus_t;

typedef struct {
  float pm10;
  float pm25;
} sdsStatus_t;

extern std::array<uint64_t, 0xff>::iterator it;
extern std::array<uint64_t, 0xff> beacons;

extern configData_t cfg;                       // current device configuration
extern char clientId[20];                      // unique clientID
extern char lmic_event_msg[LMIC_EVENTMSG_LEN]; // display buffer
extern uint8_t volatile channel;               // wifi channel rotation counter
extern uint8_t batt_level;                     // display value
extern uint16_t volatile libpax_macs_ble, libpax_macs_wifi; // libpax values
extern bool volatile TimePulseTick; // 1sec pps flag set by GPS or RTC
extern timesource_t timeSource;
extern hw_timer_t *displayIRQ, *matrixDisplayIRQ, *ppsIRQ;
extern SemaphoreHandle_t I2Caccess;
extern TaskHandle_t irqHandlerTask, ClockTask;
extern TimerHandle_t WifiChanTimer;
extern Timezone myTZ;

#endif
