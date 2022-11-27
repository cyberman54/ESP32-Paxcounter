#ifdef HAS_BUTTON

#include "globals.h"
#include "button.h"


OneButton button(HAS_BUTTON, !BUTTON_ACTIVEHIGH, !!BUTTON_PULLUP);
TaskHandle_t buttonLoopTask;

void IRAM_ATTR readButton(void) { button.tick(); }

void singleClick(void) {
#ifdef HAS_DISPLAY
  dp_refresh(true); // switch to next display page
#endif
#ifdef HAS_MATRIX_DISPLAY
  refreshTheMatrixDisplay(true); // switch to next display page
#endif
}

void longPressStart(void) {
  payload.reset();
  payload.addButton(0x01);
  SendPayload(BUTTONPORT);
}

void buttonLoop(void *parameter) {
  while (1) {
    doIRQ(BUTTON_IRQ);
    delay(50); // 50 is debounce time of OneButton lib, so doesn't hurt
  }
}

void button_init(void) {
  ESP_LOGI(TAG, "Starting button Controller...");
  xTaskCreatePinnedToCore(buttonLoop,      // task function
                          "buttonloop",    // name of task
                          2048,            // stack size of task
                          (void *)1,       // parameter of the task
                          2,               // priority of the task
                          &buttonLoopTask, // task handle
                          1);              // CPU core

  button.setPressTicks(1000);
  button.attachClick(singleClick);
  button.attachLongPressStart(longPressStart);

  attachInterrupt(digitalPinToInterrupt(HAS_BUTTON), readButton, CHANGE);
};

#endif