#ifdef HAS_BUTTON

#include "globals.h"
#include "button.h"

// Local logging tag
static const char TAG[] = "main";

void readButton() {
    ESP_LOGI(TAG, "Button pressed");
    payload.reset();
    payload.addButton(0x01);
    SendPayload(BUTTONPORT, prio_normal);
}
#endif