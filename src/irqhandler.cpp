#include "irqhandler.h"

// Local logging tag
static const char TAG[] = __FILE__;

// irq handler task, handles all our application level interrupts
void irqHandler(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  uint32_t InterruptStatus;
  static bool mask_irq = false;

  // task remains in blocked state until it is notified by an irq
  for (;;) {
    xTaskNotifyWait(0x00,             // Don't clear any bits on entry
                    ULONG_MAX,        // Clear all bits on exit
                    &InterruptStatus, // Receives the notification value
                    portMAX_DELAY);   // wait forever

    // interrupt handler to be enabled?
    if (InterruptStatus & UNMASK_IRQ)
      mask_irq = false;
    else if (mask_irq)
      continue; // suppress processing if interrupt handler is disabled

    // interrupt handler to be disabled?
    if (InterruptStatus & MASK_IRQ)
      mask_irq = true;

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

#if (TIME_SYNC_INTERVAL)
    // is time to be synced?
    if (InterruptStatus & TIMESYNC_IRQ) {
      time_t t = timeProvider();
      if (timeIsValid(t))
        setTime(t);
    }
#endif

    // is time to send the payload?
    if (InterruptStatus & SENDCYCLE_IRQ)
      sendCounter();
  }
  vTaskDelete(NULL); // shoud never be reached
}

// esp32 hardware timer triggered interrupt service routines
// they notify the irq handler task

#ifdef HAS_DISPLAY
void IRAM_ATTR DisplayIRQ() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  xTaskNotifyFromISR(irqHandlerTask, DISPLAY_IRQ, eSetBits,
                     &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR();
}
#endif

#ifdef HAS_BUTTON
void IRAM_ATTR ButtonIRQ() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  xTaskNotifyFromISR(irqHandlerTask, BUTTON_IRQ, eSetBits,
                     &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR();
}
#endif
