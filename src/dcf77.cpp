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
static const char TAG[] = "main";

// array of dcf pulses for one minute, secs 0..16 and 20 are never touched, so
// we initialize them statically to avoid dumb recalculation every minute
uint8_t DCFpulse[DCF77_FRAME_SIZE + 1] = {
    dcf_zero, dcf_zero, dcf_zero, dcf_zero, dcf_zero, dcf_zero, dcf_zero,
    dcf_zero, dcf_zero, dcf_zero, dcf_zero, dcf_zero, dcf_zero, dcf_zero,
    dcf_zero, dcf_zero, dcf_zero, dcf_zero, dcf_zero, dcf_zero, dcf_one};

// triggered by 1 second timepulse to ticker out DCF signal
void DCF_Pulse(time_t t) {

  TickType_t startTime = xTaskGetTickCount();
  uint8_t sec = second(t);

  ESP_LOGD(TAG, "DCF77 sec %d", sec);

  // induce 10 pulses
  for (uint8_t pulse = 0; pulse <= 9; pulse++) {

    switch (pulse) {

    case 0: // start of second -> start of timeframe for logic signal
      if (DCFpulse[sec] != dcf_off)
        digitalWrite(HAS_DCF77, dcf_low);
      else // 59th second reached, nothing more to do
        return;
      break;

    case 1: // 100ms after start of second -> end of timeframe for logic 0
      if (DCFpulse[sec] == dcf_zero)
        digitalWrite(HAS_DCF77, dcf_high);
      break;

    case 2: // 200ms after start of second -> end of timeframe for logic 1
      digitalWrite(HAS_DCF77, dcf_high);
      break;

    case 9: // 900ms after start -> last pulse
      return;

    } // switch

    // impulse period pause
    vTaskDelayUntil(&startTime, pdMS_TO_TICKS(DCF77_PULSE_LENGTH));

  } // for
} // DCF_Pulse()

void IRAM_ATTR DCF77_Frame(time_t tt) {

  uint8_t Parity;
  time_t t = myTZ.toLocal(tt); // convert to local time

  // ENCODE DST CHANGE ANNOUNCEMENT (Sec 16)
  DCFpulse[16] = dcf_zero; // not yet implemented

  // ENCODE DAYLIGHTSAVING (secs 17..18)
  DCFpulse[17] = myTZ.locIsDST(t) ? dcf_one : dcf_zero;
  DCFpulse[18] = myTZ.locIsDST(t) ? dcf_zero : dcf_one;

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
  Parity += dec2bcd(year(t) - 2000, 50, 57,
                    DCFpulse); // yes, we have a millenium 3000 bug here ;-)
  DCFpulse[58] = setParityBit(Parity);

  // ENCODE MARK (sec 59)
  DCFpulse[59] = dcf_off; // !! missing code here for leap second !!

  // timestamp this frame with it's minute
  DCFpulse[60] = minute(t);

} // DCF77_Frame()

// helper function to encode parity
uint8_t IRAM_ATTR setParityBit(uint8_t p) {
  return ((p & 1) ? dcf_one : dcf_zero);
}

// helper function to convert decimal to bcd digit
uint8_t IRAM_ATTR dec2bcd(uint8_t dec, uint8_t startpos, uint8_t endpos,
                          uint8_t pArray[]) {

  uint8_t data = (dec < 10) ? dec : ((dec / 10) << 4) + (dec % 10);
  uint8_t parity = 0;

  for (uint8_t n = startpos; n <= endpos; n++) {
    pArray[n] = (data & 1) ? dcf_one : dcf_zero;
    parity += (data & 1);
    data >>= 1;
  }

  return parity;
}

#endif // HAS_DCF77