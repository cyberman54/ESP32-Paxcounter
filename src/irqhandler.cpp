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
      refreshTheDisplay();
#endif

// LED Matrix display needs refresh?
#ifdef HAS_MATRIX_DISPLAY
    if (InterruptStatus & MATRIX_DISPLAY_IRQ)
      refreshTheMatrixDisplay();
#endif

// BME sensor data to be read?
#if (HAS_BME)
    if (InterruptStatus & BME_IRQ)
      bme_storedata(&bme_status);
#endif

    // are cyclic tasks due?
    if (InterruptStatus & CYCLIC_IRQ)
      doHousekeeping();

#if (TIME_SYNC_INTERVAL)
    // is time to be synced?
    if (InterruptStatus & TIMESYNC_IRQ) {
      now(); // ensure sysTime is recent
      calibrateTime();
    }
#endif

// do we have a power event?
#if (HAS_PMU)
    if (InterruptStatus & PMU_IRQ)
      pover_event_IRQ();
#endif

    // is time to send the payload?
    if (InterruptStatus & SENDCYCLE_IRQ)
      sendData();
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

#ifdef HAS_MATRIX_DISPLAY
void IRAM_ATTR MatrixDisplayIRQ() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  xTaskNotifyFromISR(irqHandlerTask, MATRIX_DISPLAY_IRQ, eSetBits,
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

#ifdef HAS_PMU
void IRAM_ATTR PMUIRQ() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  xTaskNotifyFromISR(irqHandlerTask, PMU_IRQ, eSetBits,
                     &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR();
}
#endif

void mask_user_IRQ() { xTaskNotify(irqHandlerTask, MASK_IRQ, eSetBits); }

void unmask_user_IRQ() { xTaskNotify(irqHandlerTask, UNMASK_IRQ, eSetBits); }