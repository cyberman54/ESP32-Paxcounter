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

// array of dcf pulses for one minute
uint8_t DCFpulse[DCF77_FRAME_SIZE + 1];

// triggered by 1 second timepulse to ticker out DCF signal
void DCF_Pulse(time_t t) {

  uint8_t sec = second(t);

  TickType_t startTime = xTaskGetTickCount();

  // induce 10 pulses
  for (uint8_t pulse = 0; pulse <= 9; pulse++) {

    switch (pulse) {

    case 0: // start of second -> start of timeframe for logic signal
      if (DCFpulse[sec] != dcf_off)
        set_DCF77_pin(dcf_low);
      else // 59th second reached, nothing more to do
        return;
      break;

    case 1: // 100ms after start of second -> end of timeframe for logic 0
      if (DCFpulse[sec] == dcf_zero)
        set_DCF77_pin(dcf_high);
      break;

    case 2: // 200ms after start of second -> end of timeframe for logic 1
      set_DCF77_pin(dcf_high);
      break;

    case 9: // 900ms after start -> last pulse
      return;

    } // switch

    vTaskDelayUntil(&startTime, pdMS_TO_TICKS(DCF77_PULSE_LENGTH));

  } // for
} // DCF_Pulse()

void IRAM_ATTR DCF77_Frame(time_t tt) {

  uint8_t Parity;
  time_t t = myTZ.toLocal(tt); // convert to local time

  // ENCODE HEAD
  // secs 0..19 initialized with zeros
  for (int n = 0; n <= 19; n++)
    DCFpulse[n] = dcf_zero;
  // secs 17..18: adjust for DayLightSaving
  DCFpulse[18 - (myTZ.locIsDST(t) ? 1 : 0)] = dcf_one;
  // sec 20: must be 1 to indicate time active
  DCFpulse[20] = dcf_one;

  // ENCODE MINUTE (secs 21..28)
  Parity = dec2bcd(minute(t), 21, 27, DCFpulse);
  DCFpulse[28] = (Parity & 1) ? dcf_one : dcf_zero;

  // ENCODE HOUR (secs 29..35)
  Parity = dec2bcd(hour(t), 29, 34, DCFpulse);
  DCFpulse[35] = (Parity & 1) ? dcf_one : dcf_zero;

  // ENCODE DATE (secs 36..58)
  Parity = dec2bcd(day(t), 36, 41, DCFpulse);
  Parity += dec2bcd((weekday(t) - 1) ? (weekday(t) - 1) : 7, 42, 44, DCFpulse);
  Parity += dec2bcd(month(t), 45, 49, DCFpulse);
  Parity += dec2bcd(year(t) - 2000, 50, 57,
                    DCFpulse); // yes, we have a millenium 3000 bug here ;-)
  DCFpulse[58] = (Parity & 1) ? dcf_one : dcf_zero;

  // ENCODE TAIL (sec 59)
  DCFpulse[59] = dcf_off;
  // !! missing code here for leap second !!

  // timestamp the frame with minute pointer
  DCFpulse[60] = minute(t);

  /*
    // for debug: print the DCF77 frame buffer
    char out[DCF77_FRAME_SIZE + 1];
    uint8_t i;
    for (i = 0; i < DCF77_FRAME_SIZE; i++) {
      out[i] = DCFpulse[i] + '0'; // convert int digit to printable ascii
    }
    out[DCF77_FRAME_SIZE] = '\0'; // string termination char
    ESP_LOGD(TAG, "DCF minute %d = %s", DCFpulse[DCF77_FRAME_SIZE], out);
  */
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

// helper function to switch GPIO line with DCF77 signal
void set_DCF77_pin(dcf_pinstate state) {
  switch (state) {
  case dcf_low:
#ifdef DCF77_ACTIVE_LOW
    digitalWrite(HAS_DCF77, HIGH);
#else
    digitalWrite(HAS_DCF77, LOW);
#endif
    break;
  case dcf_high:
#ifdef DCF77_ACTIVE_LOW
    digitalWrite(HAS_DCF77, LOW);
#else
    digitalWrite(HAS_DCF77, HIGH);
#endif
    break;
  } // switch
} // DCF77_pulse

#endif // HAS_DCF77