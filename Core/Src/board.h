/**
  ******************************************************************************
  * @file           : board.h
  * @brief          : the head file of the board abstract interface
  ******************************************************************************
  */
#ifndef	_BOARD_H_
#define _BOARD_H_


#ifdef __cplusplus
extern "C"
{
#endif

int bai_uart_dma_receive(void *huart, char *rx_ring, int rx_len);
int bai_uart_dma_index(void *huart, int rx_len);
int bai_uart_dma_send(void *huart, char *tx_buf, int tx_len);
int bai_uart_poll_try(void *huart);
int bai_uart_poll_receive(void *huart, char *rx_buf, int rx_len);
int bai_uart_poll_send(void *huart, char *tx_buf, int tx_len);
void bai_led_on(void *gpio, int pin);
void bai_led_off(void *gpio, int pin);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _BOARD_H_ */

