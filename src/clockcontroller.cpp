#include "clockcontroller.h"

#if defined HAS_IF482 || defined HAS_DCF77

#if defined HAS_DCF77 && defined HAS_IF482
#error You must define at most one of IF482 or DCF77!
#endif

// Local logging tag
static const char TAG[] = "main";

void clock_init(void) {

// setup clock output interface
#ifdef HAS_IF482
  IF482.begin(HAS_IF482);
#elif defined HAS_DCF77
  pinMode(HAS_DCF77, OUTPUT);
#endif

  xTaskCreatePinnedToCore(clock_loop,  // task function
                          "clockloop", // name of task
                          2048,        // stack size of task
                          (void *)1,   // task parameter
                          4,           // priority of the task
                          &ClockTask,  // task handle
                          0);          // CPU core

  assert(ClockTask); // has clock task started?
} // clock_init

void clock_loop(void *pvParameters) { // ClockTask

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  TickType_t wakeTime;
  time_t t;

#define t1(t) (t + DCF77_FRAME_SIZE + 1) // future time for next frame

// preload first DCF frame before start
#ifdef HAS_DCF77
  DCF77_Frame(t1(telegram_time()));
#endif

  // output time telegram for second following sec beginning with timepulse
  for (;;) {
    xTaskNotifyWait(0x00, ULONG_MAX, &wakeTime,
                    portMAX_DELAY); // wait for timepulse

    if (timeStatus() == timeNotSet) // do we have valid time?
      continue;

    t = telegram_time(); // time to send to clock

#if defined HAS_IF482

    IF482_Pulse(t + 1); // next second

#elif defined HAS_DCF77

    if (second(t) == DCF77_FRAME_SIZE - 1) // moment to reload frame?
      DCF77_Frame(t1(t));                  // generate next frame

    if (DCFpulse[DCF77_FRAME_SIZE] ==
        minute(t1(t)))  // do he have a recent frame?
      DCF_Pulse(t + 1); // then output next second of current frame

#endif

  } // for
} // clock_loop()

// helper function to fetch current second from most precise time source
time_t telegram_time(void) {
  time_t t;

#ifdef HAS_GPS // gps is our primary time source if present
  t = get_gpstime();
  if (t) // did we get a valid time?
    return t;
#endif

#ifdef HAS_RTC // rtc is our secondary time source if present
  t = get_rtctime();
  if (t)
    return t;
#endif

  // else we use systime as fallback source
  return now();
}

#endif // HAS_IF482 || defined HAS_DCF77