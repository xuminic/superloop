/**
  ******************************************************************************
  * @file           : ddpoll.h
  * @brief          : the head file
  ******************************************************************************
  */
#ifndef	_UART_H_
#define _UART_H_

#include "board.h"

#define	FSTAT_IDLE	0
#define FCMD_SEND	1
#define FCMD_RECV	2
#define FCMD_TX_POL	0x10	/* sending in polling mode */
#define FCMD_RX_POL	0x20	/* receiving in polling mode */


#define	UERR_NONE	0
#define UERR_INVALID	-1
#define UERR_BUSY	-2
#define UERR_WAIT	-3


typedef	struct	{
	void	*handler;	/* device handler to the HAL */
	void	*taskid;	/* recent called taskid */
	int	state;

	char	*tx_buf;	/* holding buffer for transferring */
	int	tx_len;		/* expected transferring number */
	int	tx_now;		/* currrent transferred number */

	char	*rx_buf;	/* holding buffer for receiving */
	int	rx_len;		/* expected receiving number */
	int	rx_now;		/* current received number */
	char	rx_ring[CFG_UART_RXBUF];	/* ring buffer for DMA */
} uart_t;


#ifdef __cplusplus
extern "C"
{
#endif

int uart_init(uart_t *ufp, void *huart);
int uart_write_nonblock(uart_t *ufp, char *buf, int len);
int uart_write_block(uart_t *ufp, char *buf, int len);
int uart_read_nonblock(uart_t *ufp, char *buf, int len);
int uart_read_block(uart_t *ufp, char *buf, int len);
int uart_read_timeout(uart_t *ufp, char *buf, int len, int ms);

#ifdef __cplusplus
} // __cplusplus defined.
#endif



#endif	/* _UART_H_ */

