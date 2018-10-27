#ifndef _SPISEND_H
#define _SPISEND_H

#include "globals.h"
#include "spi.h"

extern TaskHandle_t SpiTask;
extern QueueHandle_t SPISendQueue;

/*
 * Process data in SPI send queue
 */
void spi_loop(void *pvParameters);

/*
* initialize local SPI wire interface
*/
void hal_spi_init();

/*
 * Perform SPI write transaction on local SPI wire interface
 *   - write the command byte 'cmd'
 *   - write 'len' bytes out of 'buf'
 */
void hal_spi_write(uint8_t cmd, const uint8_t* buf, int len);

/*
 * Perform SPI read transaction on local SPI wire interface
 *   - read the command byte 'cmd'
 *   - read 'len' bytes into 'buf'
 */
void hal_spi_read(uint8_t cmd, uint8_t* buf, int len);

#endif