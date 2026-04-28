
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "platform.h"

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
	xtcb_t	*xtcb = bai_get_current_xtcb();

	uart_write_block(xtcb->uart, s, strlen(s));
}

int u_printf(char *fmt, ...)
{
	xtcb_t	*xtcb = bai_get_current_xtcb();
	va_list ap;
	int	n;
	
	va_start(ap, fmt);
	n = vsnprintf(xtcb->logbuf, CFG_LOG_BUFF, fmt, ap);
	va_end(ap);
	
	uart_write_block(xtcb->uart, xtcb->logbuf, n);
	return n;
}


int __io_putchar(int ch) 
{
	xtcb_t	*xtcb = bai_get_current_xtcb();
	char	buf[4];

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
	xtcb_t	*xtcb = bai_get_current_xtcb();
	char	buf[4];

	if (uart_read_block(xtcb->uart, buf, 1) < 1) {
		return -2;
	}
	
	/* echo back */
	if (buf[0] == '\n') {
		buf[0] = '\r', buf[1] = '\n';
		uart_write_block(xtcb->uart, buf, 2);
	} else {
		uart_write_block(xtcb->uart, buf, 1);
	}
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
