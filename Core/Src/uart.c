/**
  ******************************************************************************
  * @file           : uart.c
  * @brief          : the generic UART driver
  ******************************************************************************
  STM32CubeMX Configuration

  USART1: Mode Asynchronous.
  DMA Settings:
    Add USART1_TX (Stream 7, Channel 4). Mode: Normal, Increment Address: Memory.
    Add USART1_RX (Stream 5, Channel 4). Mode: Circular (best for continuous data), Increment Address: Memory.
  NVIC Settings: Enable USART1 global interrupt.
  ******************************************************************************
  */
#include <stdio.h>

#include "main.h"
#include "platform.h"


/**
 * @brief  Called when DMA finishes sending data
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) 
{
	file_t	*ufp;

	if ((ufp = ddp_whoami(huart)) != NULL) {
		ufp->state &= ~FCMD_SEND;
	}
}

/**
 * @brief  Called when DMA finishes receiving the specified number of bytes
 */
/*
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) 
{
	file_t	*ufp;

	if ((ufp = ddp_whoami(huart)) != NULL) {
		ufp->state &= ~FCMD_RECV;
	}
}
*/

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	file_t	*ufp;

	if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
		__HAL_UART_CLEAR_OREFLAG(huart);
		/* Important: Restart the interrupt here! */
		if ((ufp = ddp_whoami(huart)) != NULL) {
			HAL_UART_Receive_DMA(huart, (uint8_t*) ufp->rx_ring, CFG_UART_RXBUF);
		}
	}
}


file_t *uart_open(void *huart)
{
	file_t	*ufp;

	if ((ufp = ddp_open(huart)) == NULL) {
		return NULL;
	}

	HAL_UART_Receive_DMA(huart, (uint8_t *)ufp->rx_ring, CFG_UART_RXBUF);
	ufp->rx_now = 0;
	return ufp;
}

int uart_close(file_t *ufp)
{
	ddp_close(ufp);
	return 0;
}

int uart_write(file_t *ufp, char *buf, int len)
{
	if (!ufp || !ufp->handler) {
		return DDERR_INVALID;
	}
	if (!buf || !len) {	/* for checking the transferring status */
		return ufp->state & FCMD_SEND;
	}
	if (ufp->state & FCMD_SEND) {
		return DDERR_BUSY;
	}
	ufp->state |= FCMD_SEND;
	HAL_UART_Transmit_DMA(ufp->handler, (unsigned char *)buf, len);
	return DDERR_WAIT;
}


int uart_read(file_t *ufp, char *buf, int len)
{
	UART_HandleTypeDef *huart = ufp->handler;
	int	i, widx;

	if (!ufp || !ufp->handler) {
		return DDERR_INVALID;
	}

	/* Get the current write position from the DMA hardware
	 * CNDTR counts down from CFG_UART_RXBUF to 0 */
	widx = CFG_UART_RXBUF - __HAL_DMA_GET_COUNTER(huart->hdmarx);
	if (!buf || !len) {	/* for checking the receiving status */
		if (ufp->rx_now < widx) {
			return widx - ufp->rx_now;
		} else if (ufp->rx_now > widx) {
			return widx + CFG_UART_RXBUF - ufp->rx_now;
		}
		return 0;
	}

	/* While there is data in the ring buffer AND we haven't exceeded requested len */
	for (i = 0; (i < len) && (ufp->rx_now != widx); i++) {
		buf[i] = ufp->rx_ring[ufp->rx_now++];

		/* Wrap the read pointer if it hits the end */
		if (ufp->rx_now >= CFG_UART_RXBUF) {
			ufp->rx_now = 0;
		}
	}

	return i; /* Returns number of bytes actually read */
}


