
/*
 * cli.c
 *
 *  Created on: 19 Mar 2026
 *      Author: Andy Xu
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "led.h"
#include "platform.h"


static int cmd_help(void *taskarg, int argc, char **argv);
static int cmd_echo(void *taskarg, int argc, char **argv);

static	cli_t	cmdtab[] = {
#if	(CFG_HISTORY_ITEMS > 0)
	{ "!",    cmd_history, "history command" },
#endif
	{ "help", cmd_help, "the common help" },
	{ "echo", cmd_echo, "the echo function" },
	{ "dump", cmd_dump, "dump the memory" },
	{ "led", led_command, "LED manager" },
	{ NULL, NULL, NULL }
};


void cli_init(void *taskarg, cli_t *cmds)
{
	xtcb_t	*xtcb = taskarg;
	int	k;

	for (k = 0; k < CFG_CLI_TABLE; k++) {
		if (xtcb->cmd[k] == NULL) {
			xtcb->cmd[k] = cmds ? cmds : cmdtab;
			break;
		}
	}
}


int cli_main(void *taskarg, int argc, char **argv)
{
	xtcb_t	*xtcb = taskarg;
	cli_t	*ctab;
	int	i, k;

	for (k = 0; k < CFG_CLI_TABLE; k++) {
		if ((ctab = xtcb->cmd[k]) == NULL) {
			continue;
		}
		for (i = 0; ctab[i].func; i++) {
			if (!strcmp(ctab[i].cmd, argv[0])) {
				return ctab[i].func(taskarg, argc, argv);
			}
		}
	}
	task_printf(taskarg, "%s: command not found\r\n", argv[0]);
	return 0;
}

int cli_mkargs(char *s, char **argv, int argv_len)
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

static int cmd_help(void *taskarg, int argc, char **argv)
{
	xtcb_t  *xtcb = taskarg;
	cli_t   *ctab;
	int     i, k, n, wid = 0;

	for (k = 0; k < CFG_CLI_TABLE; k++) {
		if ((ctab = xtcb->cmd[k]) == NULL) {
			continue;
		}
		for (i = 0; ctab[i].func; i++) {
			n = strlen(ctab[i].cmd);
			wid = wid < n ? n : wid;
		}
	}
	wid = (wid + 15) / 8 * 8;

	for (k = 0; k < CFG_CLI_TABLE; k++) {
		if ((ctab = xtcb->cmd[k]) == NULL) {
			continue;
		}
		for (i = 0; ctab[i].func; i++) {
			memset(xtcb->logbuf, ' ', CFG_LOG_BUFF);
			memcpy(xtcb->logbuf, ctab[i].cmd, strlen(ctab[i].cmd));
			memcpy(xtcb->logbuf + wid, ctab[i].usage, strlen(ctab[i].usage)+1);
			strcat(xtcb->logbuf, "\r\n");
			task_puts(xtcb, xtcb->logbuf);
		}
	}
	return 0;
}


static int cmd_echo(void *taskarg, int argc, char **argv)
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


