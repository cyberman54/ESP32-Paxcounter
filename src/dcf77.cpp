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
void DCF77_Pulse(uint8_t const bit) {

  TickType_t startTime = xTaskGetTickCount();

  // induce a DCF Pulse
  for (uint8_t pulse = 0; pulse <= 2; pulse++) {

    switch (pulse) {

    case 0: // start of second -> start of timeframe for logic signal
      if (bit != dcf_Z)
        digitalWrite(HAS_DCF77, dcf_low);
      break;

    case 1: // 100ms after start of second -> end of timeframe for logic 0
      if (bit == dcf_0)
        digitalWrite(HAS_DCF77, dcf_high);
      break;

    case 2: // 200ms after start of second -> end of timeframe for logic 1
      digitalWrite(HAS_DCF77, dcf_high);
      break;

    } // switch

    // pulse pause
    vTaskDelayUntil(&startTime, pdMS_TO_TICKS(100));

  } // for
} // DCF77_Pulse()

void DCF77_Frame(const struct tm t, uint8_t *frame) {

  // writes a 1 minute dcf pulse scheme for calendar time t to frame

  uint8_t Parity;

  // START OF NEW MINUTE
  frame[0] = dcf_0;

  // PAYLOAD -> not used here
  frame[1] = dcf_0;
  frame[2] = dcf_0;
  frame[3] = dcf_0;
  frame[4] = dcf_0;
  frame[5] = dcf_0;
  frame[6] = dcf_0;
  frame[7] = dcf_0;
  frame[8] = dcf_0;
  frame[9] = dcf_0;
  frame[10] = dcf_0;
  frame[11] = dcf_0;
  frame[12] = dcf_0;
  frame[13] = dcf_0;
  frame[14] = dcf_0;
  frame[15] = dcf_0;

  // DST CHANGE ANNOUNCEMENT
  frame[16] = dcf_0; // not yet implemented

  // DAYLIGHTSAVING
  // "01" = MEZ / "10" = MESZ
  frame[17] = (t.tm_isdst > 0) ? dcf_1 : dcf_0;
  frame[18] = (t.tm_isdst > 0) ? dcf_0 : dcf_1;

  // LEAP SECOND
  frame[19] = dcf_0; // not implemented

  // BEGIN OF TIME INFORMATION
  frame[20] = dcf_1;

  // MINUTE (bits 21..28)
  Parity = dec2bcd(t.tm_min, 21, 27, frame);
  frame[28] = setParityBit(Parity);

  // HOUR (bits 29..35)
  Parity = dec2bcd(t.tm_hour, 29, 34, frame);
  frame[35] = setParityBit(Parity);

  // DATE (bits 36..58)
  Parity = dec2bcd(t.tm_mday, 36, 41, frame);
  Parity += dec2bcd((t.tm_wday == 0) ? 7 : t.tm_wday, 42, 44, frame);
  Parity += dec2bcd(t.tm_mon + 1, 45, 49, frame);
  Parity += dec2bcd(t.tm_year + 1900 - 2000, 50, 57, frame);
  frame[58] = setParityBit(Parity);

  // MARK (bit 59)
  frame[59] = dcf_Z; // !! missing code here for leap second !!

  // internal timestamp for the frame
  frame[60] = t.tm_min;

} // DCF77_Frame()

// helper function to convert decimal to bcd digit
uint8_t dec2bcd(uint8_t const dec, uint8_t const startpos, uint8_t const endpos,
                uint8_t *array) {

  uint8_t data = (dec < 10) ? dec : ((dec / 10) << 4) + (dec % 10);
  uint8_t parity = 0;

  for (uint8_t i = startpos; i <= endpos; i++) {
    array[i] = (data & 1) ? dcf_1 : dcf_0;
    parity += (data & 1);
    data >>= 1;
  }

  return parity;
}

// helper function to encode parity
uint8_t setParityBit(uint8_t const p) { return ((p & 1) ? dcf_1 : dcf_0); }

#endif // HAS_DCF77