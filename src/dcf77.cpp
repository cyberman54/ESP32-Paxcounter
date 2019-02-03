//
// source:
// https://www.elektormagazine.com/labs/dcf77-emulator-with-esp8266-elektor-labs-version-150713
//
/*
 Simulate a DCF77 radio receiver
 Emit a complete three minute pulses train from the GPIO output
 the train is preceded by a single pulse and the lacking 59th pulse to allow
 some clock model syncronization of the beginning frame. After the three pulses
 train one more single pulse is sent to safely close the frame
*/

#if defined HAS_DCF77

#include "dcf77.h"

// Local logging tag
static const char TAG[] = "main";

TaskHandle_t DCF77Task;
QueueHandle_t DCFSendQueue;
hw_timer_t *dcfCycle = NULL;

#define DCF77_FRAME_SIZE 60
#define DCF_FRAME_QUEUE_SIZE (HOMECYCLE / 60 + 1)

// array of dcf pulses for three minutes
uint8_t DCFtimeframe[DCF77_FRAME_SIZE];

// initialize and configure DCF77 output
int dcf77_init(void) {

  DCFSendQueue = xQueueCreate(DCF_FRAME_QUEUE_SIZE,
                              sizeof(DCFtimeframe) / sizeof(DCFtimeframe[0]));
  if (!DCFSendQueue) {
    ESP_LOGE(TAG, "Could not create DCF77 send queue. Aborting.");
    return 0; // failure
  }
  ESP_LOGI(TAG, "DCF77 send queue created, size %d Bytes",
           DCF_FRAME_QUEUE_SIZE * sizeof(DCFtimeframe) /
               sizeof(DCFtimeframe[0]));

  pinMode(HAS_DCF77, OUTPUT);
  digitalWrite(HAS_DCF77, LOW);

  return 1; // success

} // ifdcf77_init

// called every 100msec for DCF77 output
void DCF_Ticker() {

  static uint8_t DCF_Frame[DCF77_FRAME_SIZE];
  static uint8_t bit = 0;
  static uint8_t pulse = 0;
  static bool BitsPending = false;

  while (BitsPending) {
    switch (pulse++) {

    case 0: // start of second -> start of timeframe for logic signal
      if (DCF_Frame[bit] != dcf_off)
        digitalWrite(HAS_DCF77, LOW);
      return;

    case 1: // 100ms after start of second -> end of timeframe for logic 0
      if (DCF_Frame[bit] == dcf_zero)
        digitalWrite(HAS_DCF77, HIGH);
      return;

    case 2: // 200ms after start of second -> end of timeframe for logic signal
      digitalWrite(HAS_DCF77, HIGH);
      return;

    case 9: // last pulse before next second starts
      pulse = 0;
      if (bit++ != DCF77_FRAME_SIZE)
        return;
      else { // last pulse of DCF77 frame (59th second)
        bit = 0;
        BitsPending = false;
      };
      break;

    }; // switch
  };   // while

  // get next frame to send from queue
  if (xQueueReceive(DCFSendQueue, &DCF_Frame, (TickType_t)0) == pdTRUE)
    BitsPending = true;

} // DCF_Ticker()

void dcf77_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  // task remains in blocked state until it is notified by isr
  for (;;) {
    xTaskNotifyWait(
        0x00,      // don't clear any bits on entry
        ULONG_MAX, // clear all bits on exit
        NULL,
        portMAX_DELAY); // wait forever (missing error handling here...)

    DCF_Ticker();
  }
  vTaskDelete(DCF77Task); // shoud never be reached
} // dcf77_loop()

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

void enqueueTimeframe(time_t t) {

  uint8_t ParityCount;

  // ENCODE HEAD
  // bits 0..19 initialized with zeros
  for (int n = 0; n <= 19; n++)
    DCFtimeframe[n] = dcf_zero;
  // bits 17..18: adjust for DayLightSaving
  DCFtimeframe[18 - (myTZ.locIsDST(t) ? 1 : 0)] = dcf_one;
  // bit 20: must be 1 to indicate time active
  DCFtimeframe[20] = dcf_one;

  // ENCODE MINUTE (bits 21..28)
  ParityCount = dec2bcd(minute(t), 21, 27, DCFtimeframe);
  DCFtimeframe[28] = (ParityCount & 1) ? dcf_one : dcf_zero;

  // ENCODE HOUR (bits 29..35)
  ParityCount = dec2bcd(hour(t), 29, 34, DCFtimeframe);
  DCFtimeframe[35] = (ParityCount & 1) ? dcf_one : dcf_zero;

  // ENCODE DATE (bits 36..58)
  ParityCount = dec2bcd(day(t), 36, 41, DCFtimeframe);
  ParityCount +=
      dec2bcd((weekday(t) - 1) ? (weekday(t) - 1) : 7, 42, 44, DCFtimeframe);
  ParityCount += dec2bcd(month(t), 45, 49, DCFtimeframe);
  ParityCount +=
      dec2bcd(year(t) - 2000, 50, 57,
              DCFtimeframe); // yes, we have a millenium 3000 bug here ;-)
  DCFtimeframe[58] = (ParityCount & 1) ? dcf_one : dcf_zero;

  // ENCODE TAIL (bit 59)
  DCFtimeframe[59] = dcf_off;
  // --> missing code here for switching second!
  /*
  In unregelmäßigen Zeitabständen muss eine Schaltsekunde eingefügt werden. Dies
  ist dadurch bedingt, dass sich die Erde nicht genau in 24 Stunden um sich
  selbst dreht. Auf die koordinierte Weltzeitskala UTC bezogen, wird diese
  Korrektur zum Ende der letzten Stunde des 31. Dezember oder 30. Juni
  vorgenommen. In Mitteleuropa muss die Schaltsekunde daher am 1. Januar um 1.00
  Uhr MEZ oder am 1.Juli um 2.00 MESZ eingeschoben werden. Zu den genannten
  Zeiten werden daher 61 Sekunden gesendet.
  */

  // post generated DCFtimeframe data to DCF SendQueue
  if (xQueueSendToBack(DCFSendQueue, (void *)&DCFtimeframe[0], (TickType_t)0) !=
      pdPASS)
    ESP_LOGE(TAG, "Failed to send DCF data");

  // for debug: print the DCF77 frame buffer
  char out[DCF77_FRAME_SIZE + 1];
  uint8_t i;
  for (i = 0; i < DCF77_FRAME_SIZE; i++) {
    out[i] = DCFtimeframe[i] + '0'; // convert int digit to printable ascii
  }
  out[DCF77_FRAME_SIZE] = '\0'; // string termination char
  ESP_LOGD(TAG, "DCF=%s", out);
}

void sendDCF77() {

  time_t t = now();

  /*
    if (second(t) > 56) {
      delay(30000);
      return;
    }
  */

  // enqueue DCF timeframes for each i minute
  for (uint8_t i = 0; i < DCF_FRAME_QUEUE_SIZE; i++)
    enqueueTimeframe(t + i * 60);

  /*
    // how many to the minute end ?
    // don't forget that we begin transmission at second 58
    delay((58 - second(t)) * 1000);

    // three minutes are needed to transmit all the packet
    // then wait more 30 secs to locate safely at the half of minute
    // NB 150+60=210sec, 60secs are lost from main routine
    delay(150000);
  */

} // Ende ReadAndDecodeTime()

// interrupt service routine triggered each 100ms by ESP32 hardware timer
void IRAM_ATTR DCF77IRQ() {
  xTaskNotifyFromISR(DCF77Task, xTaskGetTickCountFromISR(), eSetBits, NULL);
  portYIELD_FROM_ISR();
}

#endif // HAS_DCF77