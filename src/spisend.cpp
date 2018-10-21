#ifdef HAS_SPI

#include "globals.h"

// Local logging tag
static const char TAG[] = "main";

MessageBuffer_t SendBuffer;

QueueHandle_t SPISendQueue;
TaskHandle_t SpiTask;

// SPI feed Task
void spi_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  while (1) {
    if (xQueueReceive(SPISendQueue, &SendBuffer, (TickType_t)0) == pdTRUE) {
      ESP_LOGI(TAG, "%d bytes sent to SPI", SendBuffer.MessageSize);
    }
    vTaskDelay(2 / portTICK_PERIOD_MS); // yield to CPU

  } // end of infinite loop

  vTaskDelete(NULL); // shoud never be reached

} // spi_loop()

#endif // HAS_SPI