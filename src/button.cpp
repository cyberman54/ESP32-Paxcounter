#ifdef HAS_BUTTON

#include "globals.h"
#include "button.h"

// Local logging tag
static const char TAG[] = __FILE__;

void readButton() {
  ESP_LOGI(TAG, "Button pressed");
#ifdef HAS_DISPLAY
  refreshtheDisplay(true); // switch to next display page
#else
  payload.reset();
  payload.addButton(0x01);
  SendPayload(BUTTONPORT, prio_normal);
#endif
}
#endif