#ifndef _SPISEND_H
#define _SPISEND_H

extern TaskHandle_t SpiTask;
extern QueueHandle_t SPISendQueue;

void spi_loop(void *pvParameters);

#endif