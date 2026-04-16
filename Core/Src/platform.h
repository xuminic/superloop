
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

#include "superloop.h"
#include "ddpoll.h"


#ifdef __cplusplus
extern "C"
{
#endif

typedef	struct	{
	char	*cmd;
	int	(*func)(int argc, char **argv);
	char	*usage;
} cli_t;


/* platform.c */
extern	file_t  *console;
extern	char	logbuf[256];

void platform_init(void *tty);
int u_puts(char *s);
int u_printf(char *fmt, ...);
void hexdump(char *s, int len);

/* cli.c */
void cli_init(void);
int cli_task(void *tcb);
int cli_main(cli_t *ctab, char *s);

/* crc16.c */
int modbus_crc16(char *buf, int len);
int modbus_crc_armour(char *buf, int len);

/* led.c */
void *led_open(int gpio);
void led_close(void *led);
int led_telegram(void *led, char *s);
int led_ticker(void *led, char *s);
int led_pwm_light(void *led, int duty);
int led_pwm_breath(void *led, int step, int ticks);
void led_tick(void);
void led_init(int gpio);
int led_command(int argc, char **argv);

/* readline.c */
char *readline(int c);

/* uart.c */
//void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
file_t *uart_open(void *huart);
int uart_close(file_t *ufp);
int uart_write(file_t *ufp, char *buf, int len);
int uart_read(file_t *ufp, char *buf, int len);


#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _SUPERLOOP_H_ */








