/*

//////////////////////// ESP32-Paxcounter \\\\\\\\\\\\\\\\\\\\\\\\\\

Copyright  2018 Christian Ambach <christian.ambach@deutschebahn.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

NOTICE:
Parts of the source files in this repository are made available under different
licenses. Refer to LICENSE.txt file in repository for more details.

*/

#ifdef HAS_SPI

#include "spislave.h"

#include <driver/spi_slave.h>
#include <sys/param.h>
#include <rom/crc.h>

static const char TAG[] = __FILE__;

#define HEADER_SIZE 4
// SPI transaction size needs to be at least 8 bytes and dividable by 4, see
// https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_slave.html
#define BUFFER_SIZE                                                            \
  (MAX(8, HEADER_SIZE + PAYLOAD_BUFFER_SIZE) +                                 \
   (PAYLOAD_BUFFER_SIZE % 4 == 0                                               \
        ? 0                                                                    \
        : 4 - MAX(8, HEADER_SIZE + PAYLOAD_BUFFER_SIZE) % 4))
DMA_ATTR uint8_t txbuf[BUFFER_SIZE];
DMA_ATTR uint8_t rxbuf[BUFFER_SIZE];

QueueHandle_t SPISendQueue;

TaskHandle_t spiTask;

void spi_slave_task(void *param) {
  while (1) {
    MessageBuffer_t msg;
    size_t transaction_size;

    // clear rx + tx buffers
    memset(txbuf, 0, sizeof(txbuf));
    memset(rxbuf, 0, sizeof(rxbuf));

    // fetch next or wait for payload to send from queue
    if (xQueueReceive(SPISendQueue, &msg, portMAX_DELAY) != pdTRUE) {
      ESP_LOGE(TAG, "Premature return from xQueueReceive() with no data!");
      continue;
    }

    // fill tx buffer with data to send from queue
    uint8_t *messageType = txbuf + 2;
    *messageType = msg.MessagePort;
    uint8_t *messageSize = txbuf + 3;
    *messageSize = msg.MessageSize;
    memcpy(txbuf + HEADER_SIZE, &msg.Message, msg.MessageSize);
    // calculate crc16 checksum over txbuf and insert checksum at pos 0+1 of
    // txbuf
    uint16_t *crc = (uint16_t *)txbuf;
    *crc = crc16_be(0, messageType, msg.MessageSize + HEADER_SIZE - 2);

    // set length for spi slave driver
    transaction_size = HEADER_SIZE + msg.MessageSize;
    // SPI transaction size needs to be at least 8 bytes and dividable by 4, see
    // https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_slave.html
    if (transaction_size % 4 != 0) {
      transaction_size += (4 - transaction_size % 4);
    }

    // prepare spi transaction
    spi_slave_transaction_t spi_transaction = {0};
    spi_transaction.length = transaction_size * 8;
    spi_transaction.tx_buffer = txbuf;
    spi_transaction.rx_buffer = rxbuf;

    // wait until spi master clocks out the data, and read results in rx buffer
    ESP_LOGI(TAG, "Prepared SPI transaction for %zu byte(s)", transaction_size);
    ESP_LOG_BUFFER_HEXDUMP(TAG, txbuf, transaction_size, ESP_LOG_DEBUG);
    ESP_ERROR_CHECK_WITHOUT_ABORT(
        spi_slave_transmit(HSPI_HOST, &spi_transaction, portMAX_DELAY));
    ESP_LOG_BUFFER_HEXDUMP(TAG, rxbuf, transaction_size, ESP_LOG_DEBUG);
    ESP_LOGI(TAG, "Transaction finished with size %zu bits",
             spi_transaction.trans_len);

    // check if command was received, then call interpreter with command payload
    if ((spi_transaction.trans_len) && ((rxbuf[2]) == RCMDPORT)) {
      rcommand(rxbuf + HEADER_SIZE, spi_transaction.trans_len - HEADER_SIZE);
    };
  }
}

esp_err_t spi_init() {
  assert(SEND_QUEUE_SIZE);
  SPISendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(MessageBuffer_t));
  if (SPISendQueue == 0) {
    ESP_LOGE(TAG, "Could not create SPI send queue. Aborting.");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "SPI send queue created, size %d Bytes",
           SEND_QUEUE_SIZE * PAYLOAD_BUFFER_SIZE);

  spi_bus_config_t spi_bus_cfg = {.mosi_io_num = SPI_MOSI,
                                  .miso_io_num = SPI_MISO,
                                  .sclk_io_num = SPI_SCLK,
                                  .quadwp_io_num = -1,
                                  .quadhd_io_num = -1,
                                  .max_transfer_sz = 0,
                                  .flags = 0};

  spi_slave_interface_config_t spi_slv_cfg = {.spics_io_num = SPI_CS,
                                              .flags = 0,
                                              .queue_size = 1,
                                              .mode = 0,
                                              .post_setup_cb = NULL,
                                              .post_trans_cb = NULL};

  // Enable pull-ups on SPI lines so we don't detect rogue pulses when no master
  // is connected
  gpio_set_pull_mode(SPI_MOSI, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(SPI_SCLK, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(SPI_CS, GPIO_PULLUP_ONLY);

  esp_err_t ret =
      spi_slave_initialize(HSPI_HOST, &spi_bus_cfg, &spi_slv_cfg, 1);

  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Starting SPIloop...");
    xTaskCreate(spi_slave_task, "spiloop", 4096, (void *)NULL, 2, &spiTask);
  } else {
    ESP_LOGE(TAG, "SPI interface initialization failed");
  }

  return ret;
}

void spi_enqueuedata(MessageBuffer_t *message) {
  // enqueue message in SPI send queue
  BaseType_t ret;
  MessageBuffer_t DummyBuffer;
  sendprio_t prio = message->MessagePrio;

  switch (prio) {
  case prio_high:
    // clear space in queue if full, then fallthrough to normal
    if (!uxQueueSpacesAvailable(SPISendQueue))
      xQueueReceive(SPISendQueue, &DummyBuffer, (TickType_t)0);
  case prio_normal:
    ret = xQueueSendToFront(SPISendQueue, (void *)message, (TickType_t)0);
    break;
  case prio_low:
  default:
    ret = xQueueSendToBack(SPISendQueue, (void *)message, (TickType_t)0);
    break;
  }
  if (ret != pdTRUE)
    ESP_LOGW(TAG, "SPI sendqueue is full");
}

void spi_queuereset(void) { xQueueReset(SPISendQueue); }

#endif // HAS_SPI