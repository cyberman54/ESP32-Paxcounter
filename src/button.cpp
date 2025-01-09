#ifdef HAS_BUTTON

#include "globals.h"
#include "button.h"
#include "configportal.h"

OneButton button(HAS_BUTTON, !BUTTON_ACTIVEHIGH, !!BUTTON_PULLUP);
TaskHandle_t buttonLoopTask;

static volatile uint8_t button_press_count = 0;
static volatile unsigned long last_button_press = 0;
#define BUTTON_PRESS_TIMEOUT 3000  // 3 seconds timeout between presses
#define BUTTON_PRESS_THRESHOLD 5   // Number of presses needed to trigger config portal

void IRAM_ATTR readButton(void) { 
    button.tick(); 
}

void singleClick(void) {
    unsigned long current_time = millis();
    
    // Reset counter if we're outside the timeout window
    if (current_time - last_button_press > BUTTON_PRESS_TIMEOUT) {
        button_press_count = 0;
    }
    
    button_press_count++;
    last_button_press = current_time;
    
    // Check if we've reached the threshold
    if (button_press_count >= BUTTON_PRESS_THRESHOLD) {
        config_portal_active = true;
        button_press_count = 0;  // Reset counter
        ESP_LOGI(TAG, "5 button presses detected, starting config portal");
    }

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

    button.setPressMs(1000);
    button.attachClick(singleClick);
    button.attachLongPressStart(longPressStart);

    attachInterrupt(digitalPinToInterrupt(HAS_BUTTON), readButton, CHANGE);
};

#endif