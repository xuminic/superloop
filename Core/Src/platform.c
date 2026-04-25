
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "main.h"
#include "platform.h"


static	tty_t	console;

static	char	*splash = "\r\n\
STM32 Time-Triggered Co-operative Super-Loop.\r\n\
#> ";

void platform_init(void *tty)
{
	tcb_t	*tcb;

	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_SET);

	// Start Transmitting: Send the initial string
	//HAL_UART_Transmit_DMA(tty, "Hell \r\n", 7);
	//HAL_Delay(100);

	led_init(GPIO_PIN_13);

	console.uart = uart_open(tty);
	readline_init(&console.readline);
	
	sloop_task_create(led_tick, 1, 0, NULL);

	uart_write(console.uart, splash, strlen(splash));
	tcb = sloop_task_create(cli_task, 10, 0, NULL);
	tcb->extension = &console;
}

tty_t *u_gettty(void)
{
	tcb_t	*tcb;
	tty_t	*tty = NULL;

	if ((tcb = sloop_get_tcb()) == NULL) {
		uart_write(console.uart, "*", 1);
		//tty = &console;
	} else if (tcb->extension == NULL) {
		uart_write(console.uart, "#", 1);
		//tty = &console;
	} else {
		tty = tcb->extension;
	}
	return tty;
}


int u_puts(char *s)
{
	tty_t	*tty;
	int	n;

	if ((tty = u_gettty()) == NULL) {
		return -1;
	}

	n = strlen(s);
	uart_write(tty->uart, s, n);
	while (tty->uart->state & FCMD_SEND) {
		sloop_dispatch();
	}
	return n;
}

int u_printf(char *fmt, ...)
{
	tty_t	*tty;
	va_list ap;
	int	n;
	
	if ((tty = u_gettty()) == NULL) {
		return -1;
	}

	va_start(ap, fmt);
	n = vsnprintf(tty->logbuf, sizeof(tty->logbuf), fmt, ap);
	va_end(ap);
	
	uart_write(tty->uart, tty->logbuf, n);
	while (tty->uart->state & FCMD_SEND) {
		sloop_dispatch();
	}
	return n;
}


int __io_putchar(int ch) 
{
	tty_t	*tty;
	char	buf[4];

	if ((tty = u_gettty()) == NULL) {
		return -1;
	}
	if (ch == '\n') {
		buf[0] = '\r', buf[1] = '\n';
		//HAL_UART_Transmit(console, (uint8_t *)buf, 2, HAL_MAX_DELAY);
		uart_write(tty->uart, buf, 2);
	} else {
		buf[0] = (char)ch;
		//HAL_UART_Transmit(console, (uint8_t *)buf, 1, HAL_MAX_DELAY);
		uart_write(tty->uart, buf, 1);
	}
	while (tty->uart->state & FCMD_SEND) {
		sloop_dispatch();
	}
	return ch;
}

int __io_getchar(void)
{
	tty_t	*tty;
	char	ch[4];

	if ((tty = u_gettty()) == NULL) {
		return -1;
	}

	/* Clear the Overrun flag (ORE) before receiving.
	 * If data was sent while the MCU wasn't listening,
	 * the UART will lock up unless this flag is cleared. */
	__HAL_UART_CLEAR_OREFLAG((UART_HandleTypeDef*)tty->uart->handler);

	/* Polling mode:
	 * - &ch: address to store the byte
	 * - 1: receive exactly 1 byte
	 * - HAL_MAX_DELAY: wait forever until a key is pressed*/
	/*if (HAL_UART_Receive(console, ch, 1, HAL_MAX_DELAY) != HAL_OK) {
		return EOF;
	}*/
	uart_read(tty->uart, ch, 1);
	while (!uart_read(tty->uart, NULL, 0)) {
		sloop_dispatch();
	}
	
	/* echo back */
	__io_putchar(ch[0]);
        return (int)ch[0];
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
