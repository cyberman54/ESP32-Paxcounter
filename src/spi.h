#ifndef _SPI_H
#define _SPI_H

extern TaskHandle_t SpiTask;
extern QueueHandle_t SPISendQueue;

void spi_loop(void *pvParameters);

#endif