#ifdef HAS_BUTTON

#include "globals.h"
#include "senddata.h"

// Local logging tag
static const char TAG[] = "main";

void IRAM_ATTR ButtonIRQ() {
  portENTER_CRITICAL(&mutexButton);
  ButtonPressedIRQ++;
  portEXIT_CRITICAL(&mutexButton);
}

void readButton() {
    portENTER_CRITICAL(&mutexButton);
    ButtonPressedIRQ = 0;
    portEXIT_CRITICAL(&mutexButton);
    ESP_LOGI(TAG, "Button pressed");
    payload.reset();
    payload.addButton(0x01);
    SendData(BUTTONPORT);
}
#endif