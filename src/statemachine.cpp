#include "statemachine.h"

// Local logging tag
static const char TAG[] = "main";

void stateMachine(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  while (1) {

#ifdef HAS_BUTTON
    if (ButtonPressedIRQ)
      readButton();
#endif

#ifdef HAS_DISPLAY
    if (DisplayTimerIRQ)
      refreshtheDisplay();
#endif

    // check housekeeping cycle and if due do the work
    if (HomeCycleIRQ)
      doHousekeeping();
    // check send cycle and if due enqueue payload to send
    if (SendCycleTimerIRQ)
      sendPayload();
    // check send queues and process due payload to send
    checkSendQueues();

    // give yield to CPU
    vTaskDelay(2 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL); // shoud never be reached
}