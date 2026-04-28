
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "led.h"
#include "platform.h"


static int cli_mkargs(char *s, char **argv, int argv_len);
static int cli_help(void *taskarg, int argc, char **argv);
static int cli_echo(void *taskarg, int argc, char **argv);
static int cli_dump(void *taskarg, int argc, char **argv);

#if	(CFG_HISTORY_ITEMS > 0)
static int cli_history(void *xtcb, int argc, char **argv);
#endif

static	cli_t	cmdtab[] = {
#if	(CFG_HISTORY_ITEMS > 0)
	{ "!",    cli_history, "history command" },
#endif
	{ "help", cli_help, "the common help" },
	{ "echo", cli_echo, "the echo function" },
	{ "dump", cli_dump, "dump the memory" },
	{ "led", led_command, "LED manager" },
	{ NULL, NULL, NULL }
};



int cli_task(void *taskarg)
{
	xtcb_t	*xtcb = taskarg;
	char	c[4], *s, *argv[CFG_CLI_MAX_PARAM];
	int	argc;

	if (uart_read_nonblock(xtcb->uart, c, 1) > 0) {
		if ((s = readline(xtcb->readline, c[0])) != NULL) {
			argc = cli_mkargs(s, argv, CFG_CLI_MAX_PARAM);
			if (argc) {
				cli_main(taskarg, cmdtab, argc, argv);
			}
			task_puts(xtcb, "#> ");
		}
	}
	return 0;
}

int cli_main(void *taskarg, cli_t *ctab, int argc, char **argv)
{
	int	i;

	for (i = 0; ctab[i].func; i++) {
		if (!strcmp(ctab[i].cmd, argv[0])) {
			return ctab[i].func(taskarg, argc, argv);
		}
		if (!strcmp(ctab[i].cmd, "+")) {
			return cli_main(taskarg, ctab[i].usage, argc, argv);
		}
	}
	task_printf(taskarg, "%s: command not found\r\n", argv[0]);
	return 0;
}

static int cli_mkargs(char *s, char **argv, int argv_len)
{
	int argc = 0;

	while (*s && (argc < argv_len)) {
        	/* skip the leading whitespaces */
		while (*s && isspace((unsigned char)*s)) *s++ = 0;

		/* mark the start position of the current parameter */
		if (*s == '"') {
			argv[argc++] = ++s;
			s = strchr(s, '"');
		} else if (*s == '\'') {
			argv[argc++] = ++s;
			s = strchr(s, '\'');
		} else if (*s == '\0') {
			break;
		} else {
			argv[argc++] = s;
			while (*s && !isspace((unsigned char)*s)) s++;
		}
		if (!s || !*s) {
			break;
		} else {
			*s++ = 0;
		}
	}
	return argc;
}

static int cli_help(void *taskarg, int argc, char **argv)
{
	xtcb_t	*xtcb = taskarg;
	cli_t	*ctab;
	int	i, n, wid;

	ctab = cmdtab;
	for (i = wid = 0; ctab[i].func; i++) {
		n = strlen(ctab[i].cmd);
		wid = wid < n ? n : wid;
	}
	wid = (wid + 15) / 8 * 8;

	for (i = 0; ctab[i].func; i++) {
		memset(xtcb->logbuf, ' ', CFG_LOG_BUFF);
		memcpy(xtcb->logbuf, ctab[i].cmd, strlen(ctab[i].cmd));
		memcpy(xtcb->logbuf + wid, ctab[i].usage, strlen(ctab[i].usage)+1);
		strcat(xtcb->logbuf, "\r\n");
		task_puts(xtcb, xtcb->logbuf);		
	}
	return 0;
}

static int cli_echo(void *taskarg, int argc, char **argv)
{
	char	*testargs[] = {
		"",
		"abc",
		" a b c ",
		"a 'b c v' 1 2 3",
		"a 'b c d",
		"a \"b c d \" e f",
		"a \" b c d",
		"a \"b c d\"e f",
		NULL
	};
	char	*myargv[16], buf[64];
	int	i, n;

	if (argc > 1) {
		for (i = 0; i < argc; i++) {
			task_printf(taskarg, "#%d: %s\n", i, argv[i]);
		}
		return 0;
	}

	for (n = 0; testargs[n]; n++) {
		task_printf(taskarg, "Parsing <%s>\n", testargs[n]);
		strcpy(buf, testargs[n]);
		argc = cli_mkargs(buf, myargv, 16);
		for (i = 0; i < argc; i++) {
			task_printf(taskarg, "    #%d: %s\n", i, myargv[i]);
		}
	}
	return 0;
}

static int cli_dump(void *taskarg, int argc, char **argv)
{
	char	*p;
	int	n;

	if (argc < 2) {
		task_puts(taskarg, "usage: dump address [length]\r\n");
		return -1;
	}

	p = (char*)strtol(argv[1], NULL, 0);
	if (argc > 2) {
		n = (int) strtol(argv[2], NULL, 0);
	} else {
		n = 16 * 4;
	}
	hexdump(taskarg, p, n);
	return 0;
}
	
#if	(CFG_HISTORY_ITEMS > 0)
static int cli_history(void *taskarg, int argc, char **argv)
{
	xtcb_t	*xtcb = taskarg;
	rdln_t	*rdl = xtcb->readline;
	char	*argxs[CFG_CLI_MAX_PARAM];
	int	i;

	if (argc < 2) {
		return readline_history_dump(rdl);
	}

	i = (int)strtol(argv[1], NULL, 0);
	if (history_copy(&rdl->history, i, rdl->lbuf, CFG_READLINE_BUFFER) > 0) {
		argc = cli_mkargs(rdl->lbuf, argxs, CFG_CLI_MAX_PARAM);
		if (argc) {
			cli_main(taskarg, cmdtab, argc, argxs);
		}
		task_puts(xtcb, "#> ");
		return 0;	
	}	
	return -2;
}
#endif

