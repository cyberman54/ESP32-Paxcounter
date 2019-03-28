/*
// Emulate a MOBATIME serial clock controller
//
// Protocol published and described here:
//
//
http://www.elektrorevue.cz/cz/download/time-distribution-within-industry-4-0-platform--controlling-slave-clocks-via-master-clock-hn50/
*/

#ifdef HAS_MOBALINE

#include "mobaline.h"

// Local logging tag
static const char TAG[] = __FILE__;

// triggered by pulse per second to ticker out mobaline frame
void MOBALINE_Pulse(time_t t, uint8_t const *DCFpulse) {

  TickType_t startTime = xTaskGetTickCount();
  uint8_t sec = second(t);

  t = myTZ.toLocal(now());
  ESP_LOGD(TAG, "[%02d:%02d:%02d.%03d] MOBALINE bit %d", hour(t), minute(t),
           second(t), millisecond(), sec);

  // induce 3 pulses
  for (uint8_t pulse = 0; pulse <= 3; pulse++) {

    switch (pulse) {

    case 0: // start of bit -> start of timeframe for logic signal
      if (DCFpulse[sec] != dcf_Z) {
        digitalWrite(HAS_DCF77, dcf_high);
        vTaskDelay(pdMS_TO_TICKS(MOBALINE_HEAD_PULSE_LENGTH));
        digitalWrite(HAS_DCF77, dcf_high);
        vTaskDelay(pdMS_TO_TICKS(MOBALINE_HEAD_PULSE_LENGTH));
        return; // next bit
      } else    // start the signalling for the next bit
        digitalWrite(HAS_DCF77, dcf_high);
      break;

    case 1: // 100ms after start of bit -> end of timeframe for logic 0
      if (DCFpulse[sec] == dcf_1)
        digitalWrite(HAS_DCF77, dcf_low);
      break;

    case 2: // 200ms after start of bit -> end of timeframe for logic 1
      if (DCFpulse[sec] == dcf_0)
        digitalWrite(HAS_DCF77, dcf_low);
      break;

    case 3: // 300ms after start -> last pulse
      break;

    } // switch

    // pulse pause
    vTaskDelayUntil(&startTime, pdMS_TO_TICKS(MOBALINE_PULSE_LENGTH));

  } // for
} // DCF77_Pulse()

uint8_t *IRAM_ATTR MOBALINE_Frame(time_t const tt) {

  // array of dcf pulses for one minute, secs 0..16 and 20 are never touched, so
  // we keep them statically to avoid same recalculation every minute

  static uint8_t DCFpulse[DCF77_FRAME_SIZE + 1];

  time_t t = myTZ.toLocal(tt); // convert to local time

  // ENCODE HEAD (bit 0))
  DCFpulse[0] = dcf_Z; // not yet implemented

  // ENCODE DAYLIGHTSAVING (bit 1)
  DCFpulse[1] = myTZ.locIsDST(t) ? dcf_1 : dcf_0;

  // ENCODE DATE (bits 2..20)
  dec2bcd(false, year(t) - 2000, 2, 9, DCFpulse);
  dec2bcd(false, month(t), 10, 14, DCFpulse);
  dec2bcd(false, day(t), 15, 20, DCFpulse);

  // ENCODE HOUR (bits 21..26)
  dec2bcd2(false, hour(t), 21, 26, DCFpulse);

  // ENCODE MINUTE (bits 27..33)
  dec2bcd2(false, minute(t), 27, 33, DCFpulse);

  // timestamp this frame with it's minute
  DCFpulse[34] = minute(t);

  return DCFpulse;

} // MOBALINE_Frame()

// helper function to convert decimal to bcd digit msb
void IRAM_ATTR dec2bcd(uint8_t const dec, uint8_t const startpos,
                       uint8_t const endpos, uint8_t *DCFpulse) {

  uint8_t data = (dec < 10) ? dec : ((dec / 10) << 4) + (dec % 10);

  for (uint8_t i = endpos; i >= startpos; i--) {
    DCFpulse[i] = (data & 1) ? dcf_1 : dcf_0;
    data >>= 1;
  }
}

#endif // HAS_MOBALINE