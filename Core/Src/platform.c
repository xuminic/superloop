
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "main.h"
#include "platform.h"


static	uart_t	conuart;
static	rdln_t	conreadln;
static	xtcb_t	console;

static	char	*splash = "\r\n\
STM32 Time-Triggered Co-operative Super-Loop.\r\n\
#> ";

void platform_init(void *huart)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_SET);

	// Start Transmitting: Send the initial string
	//HAL_UART_Transmit_DMA(tty, "Hell \r\n", 7);
	//HAL_Delay(100);

	led_init(GPIO_PIN_13);//???
	sloop_task_create(led_tick, NULL, 1, 0, NULL);

	uart_init(&conuart, huart);
	readline_init(&conreadln, &conuart);
	console.uart = &conuart;
	console.readline = &conreadln;
	console.taskid = sloop_task_create(cli_task, &console, 10, 0, NULL);

	task_puts(&console, splash);
}


uart_t *platform_find_uart(void *huart)
{
	if (console.uart->handler == huart) {
		return console.uart;
	}
	return NULL;
}


xtcb_t *platform_current_xtcb(void)
{
	tcb_t	*tcb;
	xtcb_t	*xtcb = NULL;

	if ((tcb = sloop_get_tcb()) == NULL) {
		u_puts("*");
		//xtcb = &console;
	} else if (tcb->extension == NULL) {
		u_puts("#");
		//xtcb = &console;
	} else {
		xtcb = tcb->extension;
	}
	return xtcb;
}

void task_puts(xtcb_t *xtcb, char *s)
{
	uart_write_block(xtcb->uart, s, strlen(s));
}

int task_printf(xtcb_t *xtcb, char *fmt, ...)
{
	va_list ap;
	int	n;
	
	va_start(ap, fmt);
	n = vsnprintf(xtcb->logbuf, CFG_LOG_BUFF, fmt, ap);
	va_end(ap);
	
	uart_write_block(xtcb->uart, xtcb->logbuf, n);
	return n;
}


void u_puts(char *s)
{
	uart_write_block(console.uart, s, strlen(s));
}

int u_printf(char *fmt, ...)
{
	va_list ap;
	int	n;
	
	va_start(ap, fmt);
	n = vsnprintf(console.logbuf, CFG_LOG_BUFF, fmt, ap);
	va_end(ap);
	
	uart_write_block(console.uart, console.logbuf, n);
	return n;
}


int __io_putchar(int ch) 
{
	xtcb_t	*xtcb;
	char	buf[4];

	if ((xtcb = platform_current_xtcb()) == NULL) {
		return -1;
	}
	if (ch == '\n') {
		buf[0] = '\r', buf[1] = '\n';
		uart_write_block(xtcb->uart, buf, 2);
	} else {
		buf[0] = (char)ch;
		uart_write_block(xtcb->uart, buf, 1);
	}
	return ch;
}

int __io_getchar(void)
{
	xtcb_t	*xtcb;
	char	buf[4];

	if ((xtcb = platform_current_xtcb()) == NULL) {
		return -1;
	}

	if (uart_read_block(xtcb->uart, buf, 1) < 1) {
		return -2;
	}
	
	/* echo back */
	__io_putchar(buf[0]);
        return (int)buf[0];
}


#ifdef	SIMULATION
#include <pthread.h>

static void *sim_timer(void* arg)  
{
	struct timespec sleep_ts = {0, 1000000}; /* 1ms */

	while (1) {
		nanosleep(&sleep_ts, NULL);
		sloop_tick();
	}
}

int main(int argc, char **argv)
{
	pthread_t	thread_id;
	struct timespec sleep_ts = {0, 1000000}; /* 1ms */

	pthread_create(&thread_id, NULL, sim_timer, NULL);

	platform_init(NULL);
	while (1) {
		sloop_dispatch();
		nanosleep(&sleep_ts, NULL);
	}
	return 0;
}

#endif
