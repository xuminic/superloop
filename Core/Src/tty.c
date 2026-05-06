
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "tty.h"

#define StrNCpy(d,s,n)  (strncpy((d),(s),(n)-1), (d)[(n)-1] = 0)

static void r_puts(void *tty, char *s)
{
	tty_write(tty, s, strlen(s));
}


void tty_init(tty_t *tty, void *handle)
{
	memset(tty, 0, sizeof(tty_t));
	uart_init(&tty->uart, handle);
	readline_init(&tty->readline, r_puts, tty);
}

void tty_write(tty_t *tty, void *buf, int len)
{
	uart_write_block(&tty->uart, buf, len);
}

void tty_puts(tty_t *tty, char *s)
{
	tty_write(tty, s, strlen(s));
}

int tty_printf(tty_t *tty, char *fmt, ...)
{
	va_list ap;
	int	n;
	
	va_start(ap, fmt);
	n = vsnprintf(tty->logbuf, CFG_LOG_BUFF, fmt, ap);
	va_end(ap);

	tty_write(tty, tty->logbuf, n);
	return n;
}

void tty_dump(tty_t *tty, char *prompt, char *s, int len)
{
	int	i, n, rc;

	StrNCpy(tty->logbuf, prompt, CFG_LOG_BUFF);
	i = strlen(tty->logbuf);
	n = CFG_LOG_BUFF - i;
	while (len-- && (n > 6)) {	/* big safty margin */
		rc = sprintf(tty->logbuf + i, "%02X ", (unsigned char)*s++);
		i += rc;
		n -= rc;
	}
	strcpy(tty->logbuf + i, "\r\n");
	tty_write(tty, tty->logbuf, i+2);
}


void u_puts(char *s)
{
	tty_write(bai_default_tty(), s, strlen(s));
}

int u_printf(char *fmt, ...)
{
	tty_t	*tty = bai_default_tty();
	va_list ap;
	int	n;
	
	va_start(ap, fmt);
	n = vsnprintf(tty->logbuf, CFG_LOG_BUFF, fmt, ap);
	va_end(ap);
	
	tty_write(tty, tty->logbuf, n);
	return n;
}


int __io_putchar(int ch) 
{
	char	buf[4];

	if (ch == '\n') {
		buf[0] = '\r', buf[1] = '\n';
		tty_write(bai_current_tty(), buf, 2);
	} else {
		buf[0] = (char)ch;
		tty_write(bai_current_tty(), buf, 1);
	}
	return ch;
}

int __io_getchar(void)
{
	tty_t	*tty = bai_current_tty();
	char	buf[4];

	if (uart_read_block(&tty->uart, buf, 1) < 1) {
		return -1;
	}
	
	/* echo back */
	if (buf[0] == '\n') {
		buf[0] = '\r', buf[1] = '\n';
		tty_write(tty, buf, 2);
	} else {
		tty_write(tty, buf, 1);
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
