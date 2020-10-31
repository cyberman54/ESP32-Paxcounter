#include "irqhandler.h"

// Local logging tag
static const char TAG[] = __FILE__;

// irq handler task, handles all our application level interrupts
void irqHandler(void *pvParameters) {

  _ASSERT((uint32_t)pvParameters == 1); // FreeRTOS check

  uint32_t InterruptStatus;

  // task remains in blocked state until it is notified by an irq
  for (;;) {
    xTaskNotifyWait(0x00,             // Don't clear any bits on entry
                    ULONG_MAX,        // Clear all bits on exit
                    &InterruptStatus, // Receives the notification value
                    portMAX_DELAY);   // wait forever

    if (InterruptStatus & UNMASK_IRQ) // interrupt handler to be enabled?
      InterruptStatus &= ~MASK_IRQ;   // then clear irq mask flag
    // else suppress processing if interrupt handler is disabled
    // or time critical lmic jobs are pending in next 100ms
    else if ((InterruptStatus & MASK_IRQ)
#if (HAS_LORA)
             || os_queryTimeCriticalJobs(ms2osticks(100))
#endif
    )
      continue;

// button pressed?
#ifdef HAS_BUTTON
    if (InterruptStatus & BUTTON_IRQ) {
      readButton();
      InterruptStatus &= ~BUTTON_IRQ;
    }
#endif

// display needs refresh?
#ifdef HAS_DISPLAY
    if (InterruptStatus & DISPLAY_IRQ) {
      dp_refresh();
      InterruptStatus &= ~DISPLAY_IRQ;
    }
#endif

// LED Matrix display needs refresh?
#ifdef HAS_MATRIX_DISPLAY
    if (InterruptStatus & MATRIX_DISPLAY_IRQ) {
      refreshTheMatrixDisplay();
      InterruptStatus &= ~MATRIX_DISPLAY_IRQ;
    }
#endif

#if (TIME_SYNC_INTERVAL)
    // is time to be synced?
    if (InterruptStatus & TIMESYNC_IRQ) {
      now(); // ensure sysTime is recent
      calibrateTime();
      InterruptStatus &= ~TIMESYNC_IRQ;
    }
#endif

// BME sensor data to be read?
#if (HAS_BME)
    if (InterruptStatus & BME_IRQ) {
      bme_storedata(&bme_status);
      InterruptStatus &= ~BME_IRQ;
    }
#endif

// MQTT loop due?
#if (HAS_MQTT)
    if (InterruptStatus & MQTT_IRQ) {
      mqtt_loop();
      InterruptStatus &= ~MQTT_IRQ;
    }
#endif

    // are cyclic tasks due?
    if (InterruptStatus & CYCLIC_IRQ) {
      doHousekeeping();
      InterruptStatus &= ~CYCLIC_IRQ;
    }

// do we have a power event?
#ifdef HAS_PMU
    if (InterruptStatus & PMU_IRQ) {
      AXP192_powerevent_IRQ();
      InterruptStatus &= ~PMU_IRQ;
    }
#endif

    // is time to send the payload?
    if (InterruptStatus & SENDCYCLE_IRQ) {
      sendData();
      InterruptStatus &= ~SENDCYCLE_IRQ;
    }
  } // for
} // irqHandler()

// timer triggered interrupt service routines
// they notify the irq handler task

void IRAM_ATTR doIRQ(int irq) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xTaskNotifyFromISR(irqHandlerTask, irq, eSetBits, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR();
}

#ifdef HAS_DISPLAY
void IRAM_ATTR DisplayIRQ() { doIRQ(DISPLAY_IRQ); }
#endif

#ifdef HAS_MATRIX_DISPLAY
void IRAM_ATTR MatrixDisplayIRQ() { doIRQ(MATRIX_DISPLAY_IRQ); }
#endif

#ifdef HAS_BUTTON
void IRAM_ATTR ButtonIRQ() { doIRQ(BUTTON_IRQ); }
#endif

#ifdef HAS_PMU
void IRAM_ATTR PMUIRQ() { doIRQ(PMU_IRQ); }
#endif

void mask_user_IRQ() { xTaskNotify(irqHandlerTask, MASK_IRQ, eSetBits); }

void unmask_user_IRQ() { xTaskNotify(irqHandlerTask, UNMASK_IRQ, eSetBits); }