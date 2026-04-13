/**
  ******************************************************************************
  * @file           : ddpoll.h
  * @brief          : the head file
  ******************************************************************************
  */
#ifndef	_DDPOLL_H_
#define _DDPOLL_H_

#define CFG_MAX_FILENO		4
#define CFG_UART_RXBUF		64      /* the ring buffer for UART receiving */


#define	FSTAT_IDLE	0
#define FCMD_SEND	1
#define FCMD_RECV	2
#define FCMD_TX_POL	0x10	/* sending in polling mode */
#define FCMD_RX_POL	0x20	/* receiving in polling mode */

#define FSTAT_SENT	0x100	/* finish sending */
#define FSTAT_RCVD	0x200	/* finish receiving */
#define FSTAT_CR	0x400	/* LF/CR received */
#define	FSTAT_PART	0x800	/* partial data received */


#define DDERR_NONE	0
#define DDERR_FAIL	-1
#define DDERR_BUSY	-2	/* device busy - need to wait */
#define DDERR_BREAK	-3	/* transfer break - need to start /continue */
#define DDERR_WAIT	-4
#define DDERR_INVALID	-5


typedef	struct	{
	int	(*getc)(void *);
	int	(*putc)(void *, int);
} func_t;

typedef	struct	{
	void	*handler;	/* device handler to the HAL */
	int	state;

	char	*tx_buf;	/* holding buffer for transferring */
	int	tx_len;		/* expected transferring number */
	int	tx_now;		/* currrent transferred number */

	char	*rx_buf;	/* holding buffer for receiving */
	int	rx_len;		/* expected receiving number */
	int	rx_now;		/* current received number */
	char	rx_ring[CFG_UART_RXBUF];	/* ring buffer for DMA */
	func_t	*f;		/* function group */
} file_t;


/* ddpoll.c */
void ddp_init(void *tty);
file_t *ddp_open(void *dev_id);
void ddp_close(file_t *dfp);
file_t *ddp_polling(void);
file_t *ddp_whoami(void *handler);

#endif	/* _DDPOLL_H_ */

