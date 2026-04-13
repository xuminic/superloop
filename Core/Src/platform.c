
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "main.h"
#include "platform.h"


file_t	*console;

char	logbuf[256];

static	const	char	hex_tab[] = "0123456729ABCDEF";


static int Task_Led(void *tcb);

void platform_init(void *tty)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_SET);

	// Start Transmitting: Send the initial string
	//HAL_UART_Transmit_DMA(tty, "Hell \r\n", 7);
	//HAL_Delay(100);

	console = uart_open(tty);
	
	printf("\r\nSTM32 Time-Triggered Co-operative Super-Loop.\r\n");
	sloop_task_create(Task_Led, 1, 0, NULL);

	cli_init();
	sloop_task_create(cli_task, 10, 0, NULL);
}



static uint32_t tickcnt = 0;
static int Task_Led(void *tcb)
{
	if (++tickcnt >= 1000) tickcnt = 0;
	if (tickcnt < 100) {
		HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET);
	}
	return 0;
}



int u_puts(char *s)
{
	int	n = strlen(s);

	uart_write(console, s, n);
	while (console->state & FCMD_SEND) {
		sloop_dispatch();
	}
	return n;
}

int u_printf(char *fmt, ...)
{
	va_list ap;
	int	n;

	va_start(ap, fmt);
	n = vsnprintf(logbuf, sizeof(logbuf), fmt, ap);
	va_end(ap);
	uart_write(console, logbuf, n);
	while (console->state & FCMD_SEND) {
		sloop_dispatch();
	}
	return n;
}


int __io_putchar(int ch) 
{
	char	buf[4];

	if (ch == '\n') {
		buf[0] = '\r', buf[1] = '\n';
		//HAL_UART_Transmit(console, (uint8_t *)buf, 2, HAL_MAX_DELAY);
		uart_write(console, buf, 2);
	} else {
		buf[0] = (char)ch;
		//HAL_UART_Transmit(console, (uint8_t *)buf, 1, HAL_MAX_DELAY);
		uart_write(console, buf, 1);
	}
	while (console->state & FCMD_SEND) {
		sloop_dispatch();
	}
	return ch;
}

int __io_getchar(void)
{
	char	ch[4];

	/* Clear the Overrun flag (ORE) before receiving.
	 * If data was sent while the MCU wasn't listening,
	 * the UART will lock up unless this flag is cleared. */
	__HAL_UART_CLEAR_OREFLAG((UART_HandleTypeDef*)console->handler);

	/* Polling mode:
	 * - &ch: address to store the byte
	 * - 1: receive exactly 1 byte
	 * - HAL_MAX_DELAY: wait forever until a key is pressed*/
	/*if (HAL_UART_Receive(console, ch, 1, HAL_MAX_DELAY) != HAL_OK) {
		return EOF;
	}*/
	uart_read(console, ch, 1);
	while (!uart_read(console, NULL, 0)) {
		sloop_dispatch();
	}
	
	/* echo back */
	__io_putchar(ch[0]);
        return (int)ch[0];
}


/* 00000000-  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................\r\n
   index:  0         11 (Hex)                              61 (ASCII) */
void hexdump(char *s, int len) 
{
	char	*bp;
	int 	i, n;

	bp = (char*)(((unsigned long) s) & ~0xf);
	while (bp < s + len) {
        	/* initialize the template */
        	memset(logbuf, ' ', sizeof(logbuf));
        	logbuf[8]  = '-';
        	logbuf[60] = ' ';
        	logbuf[77] = '\r';
        	logbuf[78] = '\n';
		logbuf[79] = 0;

        	/* fill the address section: if 64-bit address we only show 32 bits */
        	for (n = (int) bp, i = 7; i >= 0; i--, n >>= 4) {
			logbuf[i] = hex_tab[n & 0xf];
		}

		/* fill the hex section */
		for (i = 0; i < 16; i++, bp++) {
			if ((bp < s) || (bp >= s + len)) {
				continue;
			}
			
			/* filling the Hex part */
			n = 11 + (i * 3) + (i > 7 ? 1 : 0);
			logbuf[n]   = hex_tab[(*bp >> 4) & 0xf];
			logbuf[n+1] = hex_tab[*bp & 0xf];

			/* filling the ASCII part */
			logbuf[61 + i] = isprint((int)*bp) ? *bp : '.';
		}
		u_puts(logbuf);
	}
}

