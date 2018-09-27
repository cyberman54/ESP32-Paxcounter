#ifdef HAS_BUTTON

#include "globals.h"
#include "senddata.h"

// Local logging tag
static const char TAG[] = "main";

void IRAM_ATTR ButtonIRQ() {
  portENTER_CRITICAL(&timerMux);
  ButtonPressedIRQ++;
  portEXIT_CRITICAL(&timerMux);
}

void readButton() {
    portENTER_CRITICAL(&timerMux);
    ButtonPressedIRQ = 0;
    portEXIT_CRITICAL(&timerMux);
    ESP_LOGI(TAG, "Button pressed");
    payload.reset();
    payload.addButton(0x01);
    SendData(BUTTONPORT);
}
#endif