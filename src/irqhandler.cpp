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

    if (InterruptStatus & UNMASK_IRQ) // interrupt handler to be enabled?
      mask_irq = false;
    else if (mask_irq) // suppress processing if interrupt handler is disabled
      continue;
    else if (InterruptStatus & MASK_IRQ) { // interrupt handler to be disabled?
      mask_irq = true;
      continue;
    }

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
      ESP_LOGD(TAG, "Sync time = %d", t);
      if (timeIsValid(t))
        setTime(t);
    }
#endif

    // is time to send the payload?
    if (InterruptStatus & SENDCYCLE_IRQ)
      sendCounter();
  }
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

int mask_user_IRQ() {
  // begin of time critical section: lock I2C bus to ensure accurate timing
  if (!I2C_MUTEX_LOCK())
    return 1; // failure
  xTaskNotify(irqHandlerTask, MASK_IRQ, eSetBits);
}

int unmask_user_IRQ() {
  // end of time critical section: release I2C bus
  I2C_MUTEX_UNLOCK();
  xTaskNotify(irqHandlerTask, UNMASK_IRQ, eSetBits);
}
