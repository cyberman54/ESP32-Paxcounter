#include "clockcontroller.h"

#if defined HAS_IF482 || defined HAS_DCF77

#if defined HAS_DCF77 && defined HAS_IF482
#error You must define at most one of IF482 or DCF77!
#endif

#if (PPS < IF482_PULSE_LENGTH) || (PPS < DCF77_PULSE_LENGTH)
#error On board timepulse too fast for clockcontroller
#endif

// Local logging tag
static const char TAG[] = "main";

void clock_init(void) { // ClockTask

  timepulse_init(); // setup timepulse

// setup output interface
#ifdef HAS_IF482
  // initialize and configure IF482 Generator
  IF482.begin(HAS_IF482);
#elif defined HAS_DCF77
  // initialize and configure DCF77 output
  pinMode(HAS_DCF77, OUTPUT);
  set_DCF77_pin(dcf_low);
#endif

  xTaskCreatePinnedToCore(clock_loop,  // task function
                          "clockloop", // name of task
                          2048,        // stack size of task
                          (void *)1,   // task parameter
                          4,           // priority of the task
                          &ClockTask,  // task handle
                          0);          // CPU core

  assert(ClockTask); // has clock task started?
  timepulse_start(); // start pulse
} // clock_init

void clock_loop(void *pvParameters) { // ClockTask

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  TickType_t wakeTime, txDelay;
  uint32_t pulseCycle;
  void (*pTimeTx)(time_t); // pointer to time telegram output function

#ifdef HAS_IF482
  txDelay = pdMS_TO_TICKS(1000) - tx_Ticks(HAS_IF482);
  pulseCycle = PPS / IF482_PULSE_LENGTH;
  pTimeTx = IF482_Pulse;
#elif defined HAS_DCF77
  txDelay = pdMS_TO_TICKS(DCF77_PULSE_LENGTH);
  pulseCycle = PPS / DCF77_PULSE_LENGTH;
  pTimeTx = DCF_Pulse;
#endif

  // output time telegram triggered by timepulse
  for (;;) {
    if (timeStatus() == timeSet) // do we have valid time?
      xTaskNotifyWait(0x00, ULONG_MAX, &wakeTime,
                      portMAX_DELAY); // wait for timepulse
    for (uint8_t i = 1; i <= pulseCycle; i++) {
      pTimeTx(now());
      vTaskDelayUntil(&wakeTime, txDelay);
    }
  } // for
} // clock_loop()

#endif // HAS_DCF77 || HAS_IF482