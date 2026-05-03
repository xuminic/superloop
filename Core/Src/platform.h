
/**
  ******************************************************************************
  * @file           : platform.h
  * @brief          : the head file of board/mcu related
  ******************************************************************************
  */
#ifndef	_PLATFORM_H_
#define _PLATFORM_H_

#include "board.h"
#include "uart.h"
#include "modbus.h"
#include "readline.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef	struct	_cli_t	{
	char	*cmd;
	int	(*func)(void *xtcb, int argc, char **argv);
	void	*usage;
} cli_t;

typedef struct	_xtcb_t	{
	void	*taskid;	/* copy of the osThreadId_t */
	void	*uartid;	/* copy of the UART_HandleTypeDef handler */
	uart_t	*uart;		/* point to the UART instance */
	rdln_t	*readline;	/* point to the readline instance */
	mbus_t	*mbus;		/* point to the modbus instance */
	cli_t	*cmd[CFG_CLI_TABLE];
	char	logbuf[CFG_LOG_BUFF];
} xtcb_t;


/* platform.c */
void task_puts(xtcb_t *xtcb, char *s);
int task_printf(xtcb_t *xtcb, char *fmt, ...);
void task_write(xtcb_t *xtcb, void *buf, int len);
void task_dump(xtcb_t *xtcb, char *prompt, char *s, int len);
void u_puts(char *s);
int u_printf(char *fmt, ...);

/* hexdump.c */
void hexdump(xtcb_t *xtcb, char *s, int len);
int cmd_dump(void *taskarg, int argc, char **argv);

/* cli.c */
void cli_init(void *taskarg, cli_t *cmds);
int cli_main(void *taskarg, int argc, char **argv);
int cli_mkargs(char *s, char **argv, int argv_len);

/* crc16.c */
int modbus_crc16(char *buf, int len);
int modbus_crc_armour(char *buf, int len);


#ifdef __cplusplus
} // __cplusplus defined.
#endif

#define StrNCpy(d,s,n)	(strncpy((d),(s),(n)-1), (d)[(n)-1] = 0)

#endif	/* _SUPERLOOP_H_ */








