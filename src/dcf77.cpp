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


// triggered by second timepulse to ticker out DCF signal
void DCF77_Pulse(uint8_t bit) {
  TickType_t startTime;

  // induce a DCF Pulse
  for (uint8_t pulseLength = 0; pulseLength <= 2; pulseLength++) {
    startTime = xTaskGetTickCount(); // reference time pulse start

    switch (pulseLength) {
    case 0: // 0ms = start of pulse
      digitalWrite(HAS_DCF77, dcf_low);
      break;
    case 1: // 100ms = logic 0
      if (bit == 0)
        digitalWrite(HAS_DCF77, dcf_high);
      break;
    case 2: // 200ms = logic 1
      digitalWrite(HAS_DCF77, dcf_high);
      break;
    } // switch

    // delay to genrate pulseLength
    vTaskDelayUntil(&startTime, pdMS_TO_TICKS(100));
  } // for
} // DCF77_Pulse()

// helper function to convert decimal to bcd digit
uint64_t dec2bcd(uint8_t const dec, uint8_t const startpos,
                 uint8_t const endpos, uint8_t *parity) {
  uint8_t data = dec < 10 ? dec : ((dec / 10) << 4) + dec % 10;
  uint64_t bcd = 0;

  *parity = 0;
  for (uint8_t i = startpos; i <= endpos; i++) {
    bcd += data & 1 ? set_dcfbit(i) : 0;
    *parity ^= data & 1;
    data >>= 1;
  }

  return bcd;
}

// generates a 1 minute dcf pulse frame for calendar time t
uint64_t DCF77_Frame(const struct tm t) {
  uint8_t parity = 0, parity_sum = 0;
  uint64_t frame = 0; // start with all bits 0

  // DST CHANGE ANNOUNCEMENT (16)
  // -- not implemented --

  // DAYLIGHTSAVING  (17, 18)
  // "01" = MEZ / "10" = MESZ
  frame += t.tm_isdst > 0 ? set_dcfbit(17) : set_dcfbit(18);

  // LEAP SECOND (19)
  // -- not implemented --

  // BEGIN OF TIME INFORMATION (20)
  frame += set_dcfbit(20);

  // MINUTE (21..28)
  frame += dec2bcd(t.tm_min, 21, 27, &parity);
  frame += parity ? set_dcfbit(28) : 0;

  // HOUR (29..35)
  frame += dec2bcd(t.tm_hour, 29, 34, &parity);
  frame += parity ? set_dcfbit(35) : 0;

  // DATE (36..58)
  frame += dec2bcd(t.tm_mday, 36, 41, &parity);
  parity_sum ^= parity;
  frame += dec2bcd((t.tm_wday == 0) ? 7 : t.tm_wday, 42, 44, &parity);
  parity_sum ^= parity;
  frame += dec2bcd(t.tm_mon + 1, 45, 49, &parity);
  parity_sum ^= parity;
  frame += dec2bcd(t.tm_year + 1900 - 2000, 50, 57, &parity);
  parity_sum ^= parity;
  frame += parity_sum ? set_dcfbit(58) : 0;

  return frame;
} // DCF77_Frame()

#endif // HAS_DCF77