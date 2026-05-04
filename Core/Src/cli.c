
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

#include "board.h"
#include "platform.h"
#include "readline.h"
#include "cli.h"


static int cmd_help(void *taskarg, int argc, char **argv);
#ifdef	CFG_CLI_ECHO
static int cmd_echo(void *taskarg, int argc, char **argv);
#endif
#ifdef	CFG_CLI_DUMP
static int cmd_dump(void *taskarg, int argc, char **argv);
#endif

static	cli_t	cmddef[] = {
#if	(CFG_HISTORY_ITEMS > 0)
	{ "!",    cmd_history, "history command" },
#endif
	{ "help", cmd_help, "the common help" },
#ifdef	CFG_CLI_ECHO
	{ "echo", cmd_echo, "the echo function" },
#endif
#ifdef	CFG_CLI_DUMP
	{ "dump", cmd_dump, "dump the memory" },
#endif
	{ NULL, NULL, NULL }
};

static	cli_t	*cmdtab[CFG_CLI_MAX] = { cmddef };


int cli_init(void *taskarg, cli_t *cmds)
{
	int	k;

	for (k = 0; k < CFG_CLI_MAX; k++) {
		if (cmdtab[k] == NULL) {
			cmdtab[k] = cmds;
			return 0;
		}
	}
	return -1;	/* full */
}


int cli_main(void *taskarg, int argc, char **argv)
{
	cli_t	*ctab;
	int	i, k;

	for (k = 0; k < CFG_CLI_MAX; k++) {
		if ((ctab = cmdtab[k]) == NULL) {
			continue;
		}
		for (i = 0; ctab[i].func; i++) {
			if (!strcmp(ctab[i].cmd, argv[0])) {
				return ctab[i].func(taskarg, argc, argv);
			}
		}
	}
	task_printf(taskarg, "%s: command not found\r\n", argv[0]);
	return -1;
}

int cli_mkargs(char *s, char **argv, int argv_len)
{
	int	argc = 0;

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
	xtcb_t	*xtcb = taskarg;
	cli_t   *ctab;
	int     i, k, n, wid = 0;

	for (k = 0; k < CFG_CLI_MAX; k++) {
		if ((ctab = cmdtab[k]) == NULL) {
			continue;
		}
		for (i = 0; ctab[i].func; i++) {
			n = strlen(ctab[i].cmd);
			wid = wid < n ? n : wid;
		}
	}
	wid = (wid + 15) / 8 * 8;

	for (k = 0; k < CFG_CLI_MAX; k++) {
		if ((ctab = cmdtab[k]) == NULL) {
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

#ifdef CFG_CLI_ECHO
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
#endif	/* CFG_CLI_ECHO */


#ifdef	CFG_CLI_DUMP
static  const   char    hex_tab[] = "0123456789ABCDEF";
static	char	*hex_last = NULL;

/* 00000000-  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................\r\n
   index:  0         11 (Hex)                              61 (ASCII) */
void hexdump(void *taskarg, char *s, int len) 
{
	xtcb_t	*xtcb = taskarg;
	char	*bp, *logbuf = xtcb->logbuf;
	int 	i, n;

	bp = (char*)(((unsigned long) s) & ~0xf);
	while (bp < s + len) {
        	/* initialize the template */
        	memset(logbuf, ' ', CFG_LOG_BUFF);
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
		task_puts(xtcb, logbuf);
	}
}


static int cmd_dump(void *taskarg, int argc, char **argv)
{
	int	n = 64;

	if (argc > 1) {
		if (!strcmp(argv[1], "--help")) {
			task_puts(taskarg, "usage: dump address [length]\r\n");
			return -1;
		}
		hex_last = (char*)strtol(argv[1], NULL, 0);
	}

	if (argc > 2) {
		n = (int) strtol(argv[2], NULL, 0);
	}
	hexdump(taskarg, hex_last, n);
	hex_last += n;
	return 0;
}
#endif	/* CFG_CLI_DUMP */

