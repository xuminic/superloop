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
#include <string.h>

#include "uart.h"
#include "board.h"
#include "superloop.h"


int uart_init(uart_t *ufp, void *huart)
{
	memset(ufp, 0, sizeof(uart_t));
	ufp->handler = huart;
	ufp->rx_now = 0;
	return bai_uart_dma_receive(huart, ufp->rx_ring, CFG_UART_RXBUF);
}

int uart_write_nonblock(uart_t *ufp, char *buf, int len)
{
	if (!ufp || !ufp->handler) {
		return UERR_INVALID;
	}
	if (!buf || !len) {	/* for checking the transferring status */
		return ufp->state & FCMD_SEND;
	}
	if (ufp->state & FCMD_SEND) {
		return UERR_BUSY;
	}
	ufp->state |= FCMD_SEND;
	bai_uart_dma_send(ufp->handler, buf, len);
	return UERR_WAIT;
}

int uart_write_block(uart_t *ufp, char *buf, int len)
{
	int	rc;

	if ((rc = uart_write_nonblock(ufp, buf, len)) == UERR_WAIT) {
		while (ufp->state & FCMD_SEND) {
        	        sloop_dispatch();
        	}
		return len;
	}
	return rc;
}


int uart_read_nonblock(uart_t *ufp, char *buf, int len)
{
	int	i, widx;

	if (!ufp || !ufp->handler) {
		return UERR_INVALID;
	}

	/* Get the current write position from the DMA hardware
	 * CNDTR counts down from CFG_UART_RXBUF to 0 */
	widx = bai_uart_dma_index(ufp->handler, CFG_UART_RXBUF);
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

int uart_read_block(uart_t *ufp, char *buf, int len)
{
	int	i, n;

	if (ufp->state & FCMD_RECV) {
		return UERR_BUSY;
	}
	ufp->state |= FCMD_RECV;
	for (i = n = 0; i < len; i += n) {
		if ((n = uart_read_nonblock(ufp, buf + i, len - i)) < 0) {
			break;
		}
		if ((ufp->state & FCMD_RECV) == 0) {
			break;	/* break by interrupt */
		}
		sloop_dispatch();
	}
	ufp->state &= ~FCMD_RECV;
	return i;
}

extern	uart_t *platform_find_uart(void *huart);

void uart_write_unblock(void *huart)
{
	uart_t	*ufp;

        if ((ufp = platform_find_uart(huart)) != NULL) {
		ufp->state &= ~FCMD_SEND;
        }
}

void uart_read_unblock(void *huart)
{
	uart_t	*ufp;

        if ((ufp = platform_find_uart(huart)) != NULL) {
        	ufp->state &= ~FCMD_RECV;
        }
}

