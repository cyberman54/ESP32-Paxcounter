#ifndef UART_H
#define UART_H

// UART for Clock DCF
#define CLOCK_DCF_TXD  (GPIO_NUM_4)
#define CLOCK_DCF_RXD  (GPIO_NUM_15)
#define CLOCK_DCF_RTS  (UART_PIN_NO_CHANGE)
#define CLOCK_DCF_CTS  (UART_PIN_NO_CHANGE)
#define CLOCK_BUF_SIZE (1024)

void time_uart_send(void * pvParameters);
void time_uart_send_start();
void uart_setup();

#endif
