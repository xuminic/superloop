
/**
  ******************************************************************************
  * @file           : platform.h
  * @brief          : the head file of board/mcu related
  ******************************************************************************
  */
#ifndef	_PLATFORM_H_
#define _PLATFORM_H_

#define CFG_CLI_MAX_PARAM	16	/* maximum command line parameters */
#define CFG_CRC16_LOOKUP	1	/* require a lookup table for CRC16 (modbus) */
#define CFG_PARAM_SIZE		64	/* the parameter block size in flash sectors */
#define CFG_LOG_BUFF		256

#include "superloop.h"
#include "uart.h"
#include "led.h"
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
	void	*taskid;
	uart_t	*uart;
	rdln_t	*readline;
	cli_t	*cmd;
	char	logbuf[CFG_LOG_BUFF];
} xtcb_t;


/* platform.c */
void platform_init(void *huart);
uart_t *platform_find_uart(void *huart);
xtcb_t *platform_current_xtcb(void);
void task_puts(xtcb_t *xtcb, char *s);
int task_printf(xtcb_t *xtcb, char *fmt, ...);
void u_puts(char *s);
int u_printf(char *fmt, ...);

/* hexdump.c */
void hexdump(xtcb_t *xtcb, char *s, int len);

/* cli.c */
int cli_task(void *tcb);
int cli_main(void *taskarg, cli_t *ctab, int argc, char **argv);

/* crc16.c */
int modbus_crc16(char *buf, int len);
int modbus_crc_armour(char *buf, int len);


#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _SUPERLOOP_H_ */








