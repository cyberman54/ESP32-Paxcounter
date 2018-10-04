#include "irqhandler.h"

// Local logging tag
static const char TAG[] = "main";

// irq handler task, handles all our application level interrupts
void irqHandler(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  uint32_t InterruptStatus;

  // task remains in blocked state until it is notified by an irq
  for (;;) {
    xTaskNotifyWait(
        0x00,             // Don't clear any bits on entry
        ULONG_MAX,        // Clear all bits on exit
        &InterruptStatus, // Receives the notification value
        portMAX_DELAY);   // wait forever (missing error handling here...)

// button pressed?
#ifdef HAS_BUTTON
    if (InterruptStatus & BUTTON_IRQ)
      readButton();
#endif

// display needs refresh?
#ifdef HAS_DISPLAY
    if (InterruptStatus & DISPLAY_IRQ)
      refreshtheDisplay();
#endif

    // are cyclic tasks due?
    if (InterruptStatus & CYCLIC_IRQ)
      doHousekeeping();

    // is time to send the payload?
    if (InterruptStatus & SENDPAYLOAD_IRQ)
      sendPayload();
  }
  vTaskDelete(NULL); // shoud never be reached
}

// esp32 hardware timer triggered interrupt service routines
// they notify the irq handler task

void IRAM_ATTR ChannelSwitchIRQ() {
  xTaskNotifyGive(wifiSwitchTask);
  portYIELD_FROM_ISR();
}

void IRAM_ATTR homeCycleIRQ() {
  xTaskNotifyFromISR(irqHandlerTask, CYCLIC_IRQ, eSetBits, NULL);
  portYIELD_FROM_ISR();
}

void IRAM_ATTR SendCycleIRQ() {
  xTaskNotifyFromISR(irqHandlerTask, SENDPAYLOAD_IRQ, eSetBits, NULL);
  portYIELD_FROM_ISR();
}

#ifdef HAS_DISPLAY
void IRAM_ATTR DisplayIRQ() {
  xTaskNotifyFromISR(irqHandlerTask, DISPLAY_IRQ, eSetBits, NULL);
  portYIELD_FROM_ISR();
}
#endif

#ifdef HAS_BUTTON
void IRAM_ATTR ButtonIRQ() {
  xTaskNotifyFromISR(irqHandlerTask, BUTTON_IRQ, eSetBits, NULL);
  portYIELD_FROM_ISR();
}
#endif
