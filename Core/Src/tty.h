
/**
  ******************************************************************************
  * @file           : tty.h
  * @brief          : the head file of serial port functions
  ******************************************************************************
  */
#ifndef	_TTY_H_
#define _TTY_H_

#include "board.h"
#include "uart.h"
#include "readline.h"
#include "modbus.h"

typedef struct	{
	void	*taskid;	/* copy of the osThreadId_t */
	uart_t	uart;		/* the UART instance */
	union	{
		rdln_t	readline;	/* the readline instance */
		mbus_t	mbus;		/* the modbus instance */
	};
	char	logbuf[CFG_LOG_BUFF];
} tty_t;

#ifdef __cplusplus
extern "C"
{
#endif

void tty_init(tty_t *tty, void *handle);
void tty_puts(tty_t *tty, char *s);
int tty_printf(tty_t *tty, char *fmt, ...);
void tty_write(tty_t *tty, void *buf, int len);
void tty_dump(tty_t *tty, char *prompt, char *s, int len);
void u_puts(char *s);
int u_printf(char *fmt, ...);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _TTY_H_ */



