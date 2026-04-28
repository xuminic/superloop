/**
  ******************************************************************************
  * @file           : board.h
  * @brief          : the head file of the board abstract interface
  ******************************************************************************
  */
#ifndef	_BOARD_H_
#define _BOARD_H_

#ifdef  SIMULATOR
#include "board_simulator.h"
#else
#include "board_stm32f4xx.h"
#endif

extern	char	*splash;

#ifdef __cplusplus
extern "C"
{
#endif

void bai_uart_reset(void *huart);
int bai_uart_dma_receive(void *huart, char *rx_ring, int rx_len);
int bai_uart_dma_index(void *huart, int rx_len);
int bai_uart_dma_send(void *huart, char *tx_buf, int tx_len);
int bai_uart_poll_try(void *huart);
int bai_uart_poll_receive(void *huart, char *rx_buf, int rx_len);
int bai_uart_poll_send(void *huart, char *tx_buf, int tx_len);
void bai_led_on(void *gpio, int pin);
void bai_led_off(void *gpio, int pin);
void bai_spin_lock(void);
void bai_spin_unlock(void);
void *bai_get_uart_xtcb(void *huart);
void *bai_get_current_xtcb(void);
void bai_uart_send_sleep(void *uhandle);
void bai_uart_send_awake(void *huart);
void bai_uart_receive_sleep(void *uhandle);
void bai_uart_receive_awake(void *huart, int state);
int board_init(void);


#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _BOARD_H_ */

