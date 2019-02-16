/*
// Emulate a DCF77 radio receiver
//
// a nice & free logic test program for DCF77 can be found here:
https://www-user.tu-chemnitz.de/~heha/viewzip.cgi/hs/Funkuhr.zip/
//
*/

#ifdef HAS_DCF77

#include "dcf77.h"

// Local logging tag
static const char TAG[] = "main";

// array of dcf pulses for three minutes
uint8_t DCFpulse[DCF77_FRAME_SIZE];

// called by timepulse interrupt to ticker out DCF signal
void DCF_Pulse(time_t startTime) {

  static uint8_t current_second = second(startTime);
  static uint8_t pulse = 0;
  static bool SecondsPending = false;

  if (!SecondsPending) {
    // prepare dcf timeframe to send for next minute
    DCF77_Frame(now() + DCF77_FRAME_SIZE + 1);
    ESP_LOGD(TAG, "DCF77 minute %d", minute(now() + DCF77_FRAME_SIZE + 1));
    // begin output of dcf timeframe
    SecondsPending = true;
  }

  // ticker out current DCF frame
  if (SecondsPending) {
    switch (pulse++) {

    case 0: // start of second -> start of timeframe for logic signal
      if (DCFpulse[current_second] != dcf_off)
        set_DCF77_pin(dcf_low);
      ESP_LOGD(TAG, "DCF77 bit %d", current_second);
      break;

    case 1: // 100ms after start of second -> end of timeframe for logic 0
      if (DCFpulse[current_second] == dcf_zero)
        set_DCF77_pin(dcf_high);
      break;

    case 2: // 200ms after start of second -> end of timeframe for logic 1
      set_DCF77_pin(dcf_high);
      break;

    case 9: // 900ms after start -> last pulse before next second starts
      pulse = 0;
      if (current_second++ >=
          (DCF77_FRAME_SIZE - 1)) // end of DCF77 frame (59th second)
      {
        current_second = 0;
        SecondsPending = false;
      };
      break;

    }; // switch
  };   // if
} // DCF_Pulse()

// helper function to convert decimal to bcd digit
uint8_t dec2bcd(uint8_t dec, uint8_t startpos, uint8_t endpos,
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

void DCF77_Frame(time_t tt) {

  uint8_t Parity;
  time_t t = myTZ.toLocal(tt); // convert to local time

  // ENCODE HEAD
  // bits 0..19 initialized with zeros
  for (int n = 0; n <= 19; n++)
    DCFpulse[n] = dcf_zero;
  // bits 17..18: adjust for DayLightSaving
  DCFpulse[18 - (myTZ.locIsDST(t) ? 1 : 0)] = dcf_one;
  // bit 20: must be 1 to indicate time active
  DCFpulse[20] = dcf_one;

  // ENCODE MINUTE (bits 21..28)
  Parity = dec2bcd(minute(t), 21, 27, DCFpulse);
  DCFpulse[28] = (Parity & 1) ? dcf_one : dcf_zero;

  // ENCODE HOUR (bits 29..35)
  Parity = dec2bcd(hour(t), 29, 34, DCFpulse);
  DCFpulse[35] = (Parity & 1) ? dcf_one : dcf_zero;

  // ENCODE DATE (bits 36..58)
  Parity = dec2bcd(day(t), 36, 41, DCFpulse);
  Parity += dec2bcd((weekday(t) - 1) ? (weekday(t) - 1) : 7, 42, 44, DCFpulse);
  Parity += dec2bcd(month(t), 45, 49, DCFpulse);
  Parity += dec2bcd(year(t) - 2000, 50, 57,
                    DCFpulse); // yes, we have a millenium 3000 bug here ;-)
  DCFpulse[58] = (Parity & 1) ? dcf_one : dcf_zero;

  // ENCODE TAIL (bit 59)
  DCFpulse[59] = dcf_off;
  // !! missing code here for leap second !!

  /*
    // for debug: print the DCF77 frame buffer
    char out[DCF77_FRAME_SIZE + 1];
    uint8_t i;
    for (i = 0; i < DCF77_FRAME_SIZE; i++) {
      out[i] = DCFpulse[i] + '0'; // convert int digit to printable ascii
    }
    out[DCF77_FRAME_SIZE] = '\0'; // string termination char
    ESP_LOGD(TAG, "DCF Timeframe = %s", out);
  */
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

// helper function calculates next minute

#endif // HAS_DCF77