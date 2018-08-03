#ifdef HAS_BUTTON

#include "globals.h"
#include "senddata.h"

// Local logging tag
static const char TAG[] = "main";

void IRAM_ATTR ButtonIRQ() { ButtonPressedIRQ++; }

void readButton() {
  if (ButtonPressedIRQ) {
    portENTER_CRITICAL(&timerMux);
    ButtonPressedIRQ = 0;
    portEXIT_CRITICAL(&timerMux);
    ESP_LOGI(TAG, "Button pressed");
    payload.reset();
    payload.addButton(0x01);
    EnqueueSendData(BUTTONPORT, payload.getBuffer(), payload.getSize());
  }
}
#endif