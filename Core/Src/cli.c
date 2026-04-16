
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"


static int cli_mkargs(char *s, char **argv, int argv_len);
static int cli_help(int argc, char **argv);
static int cli_echo(int argc, char **argv);
static int cli_dump(int argc, char **argv);

static	cli_t	cmdtab[] = {
	{ "help", cli_help, "the common help" },
	{ "echo", cli_echo, "the echo function" },
	{ "dump", cli_dump, "dump the memory" },
	{ "led", led_command, "LED manager" },
	{ NULL, NULL, NULL }
};



void cli_init(void)
{
	u_puts("#> ");
}


int cli_task(void *tcb)
{
	char	c[4], *s;
	
	if (uart_read(console, c, 1)) {
		if ((s = readline(c[0])) != NULL) {
			cli_main(NULL, s);
			u_puts("#> ");
		}
	}
	return 0;
}

int cli_main(cli_t *ctab, char *s)
{
	char	*argv[CFG_CLI_MAX_PARAM];
	int	i, argc;

	if (ctab == NULL) {
		ctab = cmdtab;
	}

	argc = cli_mkargs(s, argv, CFG_CLI_MAX_PARAM);
	if (argc == 0) {
		return 0;
	}

	for (i = 0; ctab[i].func; i++) {
		if (!strcmp(ctab[i].cmd, argv[0])) {
			return ctab[i].func(argc, argv);
		}
	}
	printf("%s: command not found\n", argv[0]);
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

static int cli_help(int argc, char **argv)
{
	cli_t	*ctab;
	int	i, n, wid;

	ctab = cmdtab;
	for (i = wid = 0; ctab[i].func; i++) {
		n = strlen(ctab[i].cmd);
		wid = wid < n ? n : wid;
	}
	wid = (wid + 15) / 8 * 8;

	for (i = 0; ctab[i].func; i++) {
		memset(logbuf, ' ', sizeof(logbuf));
		memcpy(logbuf, ctab[i].cmd, strlen(ctab[i].cmd));
		memcpy(logbuf + wid, ctab[i].usage, strlen(ctab[i].usage)+1);
		strcat(logbuf, "\r\n");
		u_puts(logbuf);		
	}
	return 0;
}

static int cli_echo(int argc, char **argv)
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
			printf("#%d: %s\n", i, argv[i]);
		}
		return 0;
	}

	for (n = 0; testargs[n]; n++) {
		printf("Parsing <%s>\n", testargs[n]);
		strcpy(buf, testargs[n]);
		argc = cli_mkargs(buf, myargv, 16);
		for (i = 0; i < argc; i++) {
			printf("    #%d: %s\n", i, myargv[i]);
		}
	}
	return 0;
}

static int cli_dump(int argc, char **argv)
{
	char	*p;
	int	n;

	if (argc < 2) {
		u_puts("usage: dump address [length]\r\n");
		return -1;
	}

	p = (char*)strtol(argv[1], NULL, 0);
	if (argc > 2) {
		n = (int) strtol(argv[2], NULL, 0);
	} else {
		n = 16 * 4;
	}
	hexdump(p, n);
	return 0;
}
	
