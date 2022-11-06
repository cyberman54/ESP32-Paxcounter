#include "irqhandler.h"

// Local logging tag
static const char TAG[] = __FILE__;

TaskHandle_t irqHandlerTask = NULL;

// irq handler task, handles all our application level interrupts
void irqHandler(void *pvParameters) {
  _ASSERT((uint32_t)pvParameters == 1); // FreeRTOS check

  uint32_t irqSource;

  // task remains in blocked state until it is notified by an irq
  for (;;) {
    xTaskNotifyWait(0x00,           // Don't clear any bits on entry
                    ULONG_MAX,      // Clear all bits on exit
                    &irqSource,     // Receives the notification value
                    portMAX_DELAY); // wait forever

    if (irqSource & UNMASK_IRQ) // interrupt handler to be enabled?
      irqSource &= ~MASK_IRQ;   // then clear irq mask flag
    // else suppress processing if interrupt handler is disabled
    // or time critical lmic jobs are pending in next 100ms
    else if ((irqSource & MASK_IRQ)
#if (HAS_LORA)
             || os_queryTimeCriticalJobs(ms2osticks(100))
#endif
    )
      continue;

// button pressed?
#ifdef HAS_BUTTON
    if (irqSource & BUTTON_IRQ)
      readButton();
#endif

// display needs refresh?
#ifdef HAS_DISPLAY
    if (irqSource & DISPLAY_IRQ)
      dp_refresh();
#endif

// LED Matrix display needs refresh?
#ifdef HAS_MATRIX_DISPLAY
    if (irqSource & MATRIX_DISPLAY_IRQ)
      refreshTheMatrixDisplay();
#endif

#if (TIME_SYNC_INTERVAL)
    // is time to be synced?
    if (irqSource & TIMESYNC_IRQ) {
      calibrateTime();
    }
#endif

// BME sensor data to be read?
#if (HAS_BME)
    if (irqSource & BME_IRQ)
      bme_storedata(&bme_status);
#endif

    // are cyclic tasks due?
    if (irqSource & CYCLIC_IRQ)
      doHousekeeping();

// do we have a power event?
#ifdef HAS_PMU
    if (irqSource & PMU_IRQ)
      AXP192_powerevent_IRQ();
#endif

    // is time to send the payload?
    if (irqSource & SENDCYCLE_IRQ) {
      sendData();
      // goto sleep if we have a sleep cycle
      if (cfg.sleepcycle)
#ifdef HAS_BUTTON
        enter_deepsleep(cfg.sleepcycle * 10, (gpio_num_t)HAS_BUTTON);
#else
        enter_deepsleep(cfg.sleepcycle * 10);
#endif
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

void mask_user_IRQ() { xTaskNotify(irqHandlerTask, MASK_IRQ, eSetBits); }

void unmask_user_IRQ() { xTaskNotify(irqHandlerTask, UNMASK_IRQ, eSetBits); }