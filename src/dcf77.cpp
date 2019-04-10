/*
// Emulate a DCF77 radio receiver to control an external clock
//
// a nice & free logic test program for DCF77 can be found here:
https://www-user.tu-chemnitz.de/~heha/viewzip.cgi/hs/Funkuhr.zip/
//
// a DCF77 digital scope for Arduino boards can be found here:
https://github.com/udoklein/dcf77
//
*/

#ifdef HAS_DCF77

#include "dcf77.h"

// Local logging tag
static const char TAG[] = __FILE__;

// triggered by second timepulse to ticker out DCF signal
void DCF77_Pulse(time_t t, uint8_t const *DCFpulse) {

  TickType_t startTime = xTaskGetTickCount();
  uint8_t sec = second(t);

  t = myTZ.toLocal(now());
  ESP_LOGD(TAG, "[%02d:%02d:%02d.%03d] DCF second %d", hour(t), minute(t),
           second(t), millisecond(), sec);

  // induce a DCF Pulse
  for (uint8_t pulse = 0; pulse <= 2; pulse++) {

    switch (pulse) {

    case 0: // start of second -> start of timeframe for logic signal
      if (DCFpulse[sec] != dcf_Z)
        digitalWrite(HAS_DCF77, dcf_low);
      break;

    case 1: // 100ms after start of second -> end of timeframe for logic 0
      if (DCFpulse[sec] == dcf_0)
        digitalWrite(HAS_DCF77, dcf_high);
      break;

    case 2: // 200ms after start of second -> end of timeframe for logic 1
      digitalWrite(HAS_DCF77, dcf_high);
      break;

    } // switch

    // pulse pause
    vTaskDelayUntil(&startTime, pdMS_TO_TICKS(DCF77_PULSE_LENGTH));

  } // for
} // DCF77_Pulse()

uint8_t *IRAM_ATTR DCF77_Frame(time_t const tt) {

  // array of dcf pulses for one minute, secs 0..16 and 20 are never touched, so
  // we keep them statically to avoid same recalculation every minute

  static uint8_t DCFpulse[DCF77_FRAME_SIZE + 1] = {
      dcf_0, dcf_0, dcf_0, dcf_0, dcf_0, dcf_0, dcf_0,
      dcf_0, dcf_0, dcf_0, dcf_0, dcf_0, dcf_0, dcf_0,
      dcf_0, dcf_0, dcf_0, dcf_0, dcf_0, dcf_0, dcf_1};

  uint8_t Parity;
  time_t t = myTZ.toLocal(tt); // convert to local time

  // ENCODE DST CHANGE ANNOUNCEMENT (Sec 16)
  DCFpulse[16] = dcf_0; // not yet implemented

  // ENCODE DAYLIGHTSAVING (secs 17..18)
  DCFpulse[17] = myTZ.locIsDST(t) ? dcf_1 : dcf_0;
  DCFpulse[18] = myTZ.locIsDST(t) ? dcf_0 : dcf_1;

  // ENCODE MINUTE (secs 21..28)
  Parity = dec2bcd(minute(t), 21, 27, DCFpulse);
  DCFpulse[28] = setParityBit(Parity);

  // ENCODE HOUR (secs 29..35)
  Parity = dec2bcd(hour(t), 29, 34, DCFpulse);
  DCFpulse[35] = setParityBit(Parity);

  // ENCODE DATE (secs 36..58)
  Parity = dec2bcd(day(t), 36, 41, DCFpulse);
  Parity += dec2bcd((weekday(t) - 1) ? (weekday(t) - 1) : 7, 42, 44, DCFpulse);
  Parity += dec2bcd(month(t), 45, 49, DCFpulse);
  Parity += dec2bcd(year(t) - 2000, 50, 57, DCFpulse);
  DCFpulse[58] = setParityBit(Parity);

  // ENCODE MARK (sec 59)
  DCFpulse[59] = dcf_Z; // !! missing code here for leap second !!

  // timestamp this frame with it's minute
  DCFpulse[60] = minute(t);

  return DCFpulse;

} // DCF77_Frame()

// helper function to convert decimal to bcd digit
uint8_t IRAM_ATTR dec2bcd(uint8_t const dec, uint8_t const startpos,
                          uint8_t const endpos, uint8_t *DCFpulse) {

  uint8_t data = (dec < 10) ? dec : ((dec / 10) << 4) + (dec % 10);
  uint8_t parity = 0;

  for (uint8_t i = startpos; i <= endpos; i++) {
    DCFpulse[i] = (data & 1) ? dcf_1 : dcf_0;
    parity += (data & 1);
    data >>= 1;
  }

  return parity;
}

// helper function to encode parity
uint8_t IRAM_ATTR setParityBit(uint8_t const p) {
  return ((p & 1) ? dcf_1 : dcf_0);
}

#endif // HAS_DCF77