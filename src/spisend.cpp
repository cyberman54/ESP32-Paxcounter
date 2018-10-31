#ifdef HAS_SPI

#include "spisend.h"

// Local logging tag
static const char TAG[] = "main";

MessageBuffer_t SendBuffer;

QueueHandle_t SPISendQueue;
TaskHandle_t SpiTask;

// SPI feed Task
void spi_loop(void *pvParameters) {

  uint8_t buf[32];

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  while (1) {
    // check if data to send on SPI interface
    if (xQueueReceive(SPISendQueue, &SendBuffer, (TickType_t)0) == pdTRUE) {
      hal_spi_write(SendBuffer.MessagePort, SendBuffer.Message,
                    SendBuffer.MessageSize);
      ESP_LOGI(TAG, "%d bytes sent to SPI", SendBuffer.MessageSize);
    }

    // check if command is received on SPI command port, then call interpreter
    hal_spi_read(RCMDPORT, buf, 32);
    if (buf[0])
      rcommand(buf, sizeof(buf));

    vTaskDelay(2 / portTICK_PERIOD_MS); // yield to CPU
  }                                     // end of infinite loop

  vTaskDelete(NULL); // shoud never be reached

} // spi_loop()

// SPI hardware abstraction layer

void hal_spi_init() { SPI.begin(SCK, MISO, MOSI, SS); }

void hal_spi_trx(u1_t cmd, u1_t *buf, int len, u1_t is_read) {

  u1_t nss = SS;
  SPISettings settings(1E6, MSBFIRST, SPI_MODE0);
  SPI.beginTransaction(settings);

  digitalWrite(nss, 0);

  SPI.transfer(cmd);

  for (u1_t i = 0; i < len; i++) {
    u1_t *p = buf + i;
    u1_t data = is_read ? 0x00 : *p;
    data = SPI.transfer(data);
    if (is_read)
      *p = data;
  }

  digitalWrite(nss, 1);
  SPI.endTransaction();
}

void hal_spi_write(u1_t cmd, const u1_t *buf, int len) {
  hal_spi_trx(cmd, (u1_t *)buf, len, 0);
}

void hal_spi_read(u1_t cmd, u1_t *buf, int len) {
  hal_spi_trx(cmd, buf, len, 1);
}

#endif // HAS_SPI