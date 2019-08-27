#include <Arduino.h>
#include <sys/time.h>

#include <lmic/oslmic.h>
#include "globals.h"
#include "driver/uart.h"

static const char TAG[] = __FILE__;
TaskHandle_t UartTask = NULL;

void time_uart_send(void * pvParameters) {
  struct timeval curTime;

  const char* format =   "\bOAL%y%m%dF%H%M%S\r";
  char timestamp[64] = {0};
  TickType_t xLastWakeTime = xTaskGetTickCount();

  for(;;) {
    time_t nowTime = now();
    strftime(timestamp, sizeof(timestamp), format, localtime(&nowTime));
    ESP_LOGI(TAG, "Current Time:  %s", timestamp);

    int len = strlen(timestamp)+1;

    // Send Data via UART
    uart_write_bytes(UART_NUM_1, timestamp, len);

    // gettimeofday(&curTime, &tz);
    int sleep = 1000 - (curTime.tv_usec/1000);
    ostime_t now = os_getTime();
    ESP_LOGD(TAG, "Sleep Time:  %d, now: %d\n", sleep, now);
    vTaskDelayUntil( &xLastWakeTime, (TickType_t)(sleep/portTICK_PERIOD_MS) );

    // Read UART for testing purposes
    /**
    uint8_t *data = (uint8_t *) malloc(CLOCK_BUF_SIZE);

    // Read Data via UART
    uart_read_bytes(UART_NUM_1, data, CLOCK_BUF_SIZE, 20 / portTICK_RATE_MS);
    const char * dataC = (const char *) data;
    ESP_LOGI(TAG, "Data Read: %s", dataC);

    free(data);
    **/
  }

  vTaskDelete(NULL);
}

void time_uart_send_start() {
  if (UartTask) {
    return;
  }
  xTaskCreatePinnedToCore(time_uart_send,       // task function
                          "time_uart_send",     // name of task
                          2048,                 // stack size of task
                          (void *)1,            // parameter of the task
                          2,                    // priority of the task
                          &UartTask,           // task handle
                          1);                   // CPU core
}
