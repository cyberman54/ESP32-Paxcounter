#if defined HAS_IF482 && defined HAS_RTC

/*

IF482 Generator to control clocks with IF482 telegram input (e.g. BÜRK BU190)
   
Example IF482 telegram: "OAL160806F170400"

IF482 Specification:
http://www.mobatime.com/fileadmin/user_upload/downloads/TE-112023.pdf

The IF 482 telegram is a time telegram, which sends the time and date
information as ASCII characters through the serial interface RS 232 or RS 422.

Communication parameters:

Baud rate: 9600 Bit/s
Data bits 7
Parity: even
Stop bit: 1
Jitter: < 50ms

Interface : RS232 or RS422

Synchronization: Telegram ends at the beginning of the second
specified in the telegram

Cycle: 1 second

Format of ASCII telegram string:

Byte  Meaning             ASCII     Hex
 1    Start of telegram   O         4F
 2    Monitoring*         A         41
 3    Time-Season**       W/S/U/L   57 or 53
 4    Year tens           0 .. 9    30 .. 39
 5    Year unit           0 .. 9    30 .. 39
 6    Month tens          0 or 1    30 or 31
 7    Month unit          0 .. 9    30 .. 39
 8    Day tens            0 .. 3    30 .. 33
 9    Day unit            0 .. 9    30 .. 39
10    Day of week***      1 .. 7    31 .. 37
11    Hours tens          0 .. 2    30 .. 32
12    Hours unit          0 .. 9    30 .. 39
13    Minutes tens        0 .. 5    30 .. 35
14    Minutes unit        0 .. 9    30 .. 39
15    Seconds tens        0 .. 5    30 .. 35
16    Seconds unit        0 .. 9    30 .. 39
17    End of telegram     CR        0D

*) Monitoring:
With a correctly received time in the sender unit, the ASCII character 'A' is
issued. If 'M' is issued, this indicates that the sender was unable to receive
any time signal for over 12 hours (time is accepted with ‘A’ and ‘M’).

**) Season:
W: Standard time,
S: Season time,
U: UTC time (not supported by all systems),
L: Local Time

***) Day of week:
not evaluated by model BU-190

*/

#include "if482.h"

// Local logging tag
static const char TAG[] = "main";

HardwareSerial IF482(1); // use UART #1

// initialize and configure GPS
void if482_init(void) {

  // open serial interface
  IF482.begin(HAS_IF482);

  // use rtc 1Hz clock for triggering IF482 telegram send
  Rtc.SetSquareWavePinClockFrequency(DS3231SquareWaveClock_1Hz);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock);
  pinMode(RTC_INT, INPUT_PULLUP);

  ESP_LOGI(TAG, "IF482 generator initialized");

} // if482_init

String if482Telegram(time_t t) {

  char mon;
  char buf[14] = "000000F000000";
  char out[17];

  switch (timeStatus()) { // indicates if time has been set and recently synced

  case timeSet: // time is set and is synced
    mon = 'A';
    break;

  case timeNeedsSync: // time had been set but sync attempt did not succeed
    mon = 'M';
    break;

  default: // time not set, no valid time
    mon = '?';
    break;

  } // switch

  if (!timeNotSet) // do we have valid time?
    snprintf(buf, sizeof buf, "%02u%02u%02u%1u%02u%02u%02u", year(t)-2000, month(t),
             day(t), weekday(t), hour(t), minute(t), second(t));

  snprintf(out, sizeof out, "O%cL%s\r", mon, buf);

  return out;
}

// interrupt triggered routine
void sendIF482(time_t t) { IF482.print(if482Telegram(t)); }

#endif // HAS_IF482