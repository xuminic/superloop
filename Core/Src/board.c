/**
  ******************************************************************************
  * @file           : board.c
  * @brief          : the board abstract interface
  ******************************************************************************
*/

#ifdef	SIMULATOR
#include "board_simulator.cxx"
#else
#include "board_stm32f4xx.cxx"
#endif
#include <string.h>

int uart_read_poll(uart_t *ufp, char *buf, int len)
{
	int	i, rc;

	for (i = 0; i < len; i++) {
		while ((rc = bai_uart_poll_try(ufp->handler)) < 0) {
			sloop_dispatch();
		}
		*buf++ = (char)rc;
	}
	return len;
}

static int cli_loop(xtcb_t *xtcb, int ms)
{
	char	*s, c[4], *argv[CFG_CLI_MAX_PARAM];
	int	argc;

	if (uart_read_nonblock(xtcb->uart, c, 1) > 0) {
		if ((s = readline(xtcb->readline, c[0])) != NULL) {
			argc = cli_mkargs(s, argv, CFG_CLI_MAX_PARAM);
			if (argc) {
				if (!strcmp(argv[0], "exit")) {
					return -2;	/* exit */
				}
				cli_main(xtcb, argc, argv);
			}
			task_puts(xtcb, "#> ");
		}
		return 0;	/* reset timer */
	}
	return -1;	/* wait */
}


int task_commandline(void *taskarg)
{
	return cli_loop(taskarg, 0);
}

