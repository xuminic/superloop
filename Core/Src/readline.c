
/*
VT100: https://gist.github.com/delameter/b9772a0bf19032f977b985091f0eb5c1
       https://www2.ccs.neu.edu/research/gpc/VonaUtils/vona/terminal/vtansi.htm

cursorup(n) CUU       Move cursor up n lines                 ^[[<n>A
cursordn(n) CUD       Move cursor down n lines               ^[[<n>B
cursorrt(n) CUF       Move cursor right n lines              ^[[<n>C
cursorlf(n) CUB       Move cursor left n lines               ^[[<n>D
cleareol EL0          Clear line from cursor right           ^[[K
cleareol EL0          Clear line from cursor right           ^[[0K
clearbol EL1          Clear line from cursor left            ^[[1K
clearline EL2         Clear entire line                      ^[[2K
cleareos ED0          Clear screen from cursor down          ^[[J
cleareos ED0          Clear screen from cursor down          ^[[0J
clearbos ED1          Clear screen from cursor up            ^[[1J
clearscreen ED2       Clear entire screen                    ^[[2J
modesoff SGR0         Turn off character attributes          ^[[m
modesoff SGR0         Turn off character attributes          ^[[0m

Set Attribute Mode	<ESC>[{attr1};...;{attrn}m
    0	Reset all attributes
    1	Bright
    2	Dim
    4	Underscore
    5	Blink
    7	Reverse
    8	Hidden

    	Foreground Colours
    30	Black
    31	Red
    32	Green
    33	Yellow
    34	Blue
    35	Magenta
    36	Cyan
    37	White

    	Background Colours
    40	Black
    41	Red
    42	Green
    43	Yellow
    44	Blue
    45	Magenta
    46	Cyan
    47	White

VT220:
Home		ESC [ 1 ~ (or ESC [ H)
Insert		ESC [ 2 ~
Delete		ESC [ 3 ~
End		ESC [ 4 ~ (or ESC [ F)
PageUp		ESC [ 5 ~
PageDown	ESC [ 6 ~

POSIX (IEEE Std 1003.1)  Control Characters
Backspace	0x08 (BS) / 0x7F (DEL)	
Ctrl-U	删除整行	0x15 (NAK)	VKILL
Ctrl-W	删除单词	0x17 (ETB)	VWERASE
Ctrl-A	移至行首	0x01 (SOH)	(常用但在 Readline 中定义)
Ctrl-E	移至行尾	0x05 (ENQ)	(常用但在 Readline 中定义)
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "readline.h"
#include "platform.h"


#define RDL_APPEND	0
#define RDL_HOLD	1
#define RDL_STATMSK	0xf
#define RDL_REPLACE	0x10	/* 0: insert mode 1: replace mode */

#define GETSTAT(n)	((n) & RDL_STATMSK)
#define SETSTAT(n,s)	(((n) & ~RDL_STATMSK) | (s))


#ifdef	EXECUTABLE
#include <unistd.h>
#define	r_puts(n,s)	write(STDOUT_FILENO, (s), strlen(s))
#else
static void r_puts(rdln_t *rdl, char *s)
{
	uart_write_block(rdl->uart, s, strlen(s));
}
#endif


static int readline_uphold(rdln_t *rdl);
static int readline_insert(rdln_t *rdl, int c);
static int readline_render(rdln_t *rdl, int curmove);
static int readline_dummy(rdln_t *rdl);
static int readline_backspace(rdln_t *rdl);
static int readline_delete(rdln_t *rdl);
static int readline_kill(rdln_t *rdl);
static int readline_word_erase(rdln_t *rdl);
static int readline_cursor_left(rdln_t *rdl);
static int readline_cursor_right(rdln_t *rdl);
static int readline_cursor_home(rdln_t *rdl);
static int readline_cursor_end(rdln_t *rdl);

#if	(CFG_HISTORY_ITEMS > 0)
static int readline_history_up(rdln_t *rdl);
static int readline_history_down(rdln_t *rdl);
#endif
	
static	struct	{
	char	*cmd;
	int	len;
	int	(*func)(rdln_t *rdl);
} line_cmd[] = {
	{ "\x8",	1, 	readline_backspace },	/* Ctrl-H / Backspace */
	{ "\x7f",	1,	readline_backspace },
	{ "\x15",	1,	readline_kill },	/* Ctrl-U */
	{ "\x1",	1,	readline_cursor_home },	/* Ctrl-A */
	{ "\x5",	1,	readline_cursor_end },	/* Ctrl-E */
	{ "\x17",	1,	readline_word_erase },	/* Ctrl-W */
	{ "\x1b[1~",	4,	readline_cursor_home },	/* Control Sequence Introducer (CSI) */
	{ "\x1bO1~",	4,	readline_cursor_home },	/* Single Shift Select (SS3) */
	{ "\x1b[H",	3,	readline_cursor_home },
	{ "\x1bOH",	3,	readline_cursor_home },
	{ "\x1b[4~",	4,	readline_cursor_end },
	{ "\x1bO4~",	4,	readline_cursor_end },
	{ "\x1b[F",	3,	readline_cursor_end },
	{ "\x1bOF",	3,	readline_cursor_end },
	{ "\x1b[C",	3,	readline_cursor_right },
	{ "\x1bOC",	3,	readline_cursor_right },
	{ "\x1b[D",	3,	readline_cursor_left },
	{ "\x1bOD",	3,	readline_cursor_left },
	{ "\x1b[3~",	4,	readline_delete },
	{ "\x1bO3~",	4,	readline_delete },
	{ "\x1b[2~",	4,	readline_dummy },	/* Insert */
#if	(CFG_HISTORY_ITEMS > 0)
	{ "\x6",        1,      readline_history_dump }, /* Ctrl-F */
	{ "\x1b[5~",	4,	readline_history_up }, 	/* PageUp */
	{ "\x1bO5~",	4,	readline_history_up }, 	/* PageUp */
	{ "\x1b[6~",	4,	readline_history_down }, /* PageDown */
	{ "\x1bO6~",	4,	readline_history_down }, /* PageDown */
	{ "\x1b[A",	3,	readline_history_up },
	{ "\x1bOA",	3,	readline_history_up },
	{ "\x1b[B",	3,	readline_history_down },
	{ "\x1bOB",	3,	readline_history_down },
#endif
	{ 0, 0, NULL }
};


void *readline_init(rdln_t *rdl, void *uart)
{
	memset(rdl, 0, sizeof(rdln_t));
	rdl->uart = uart;
#if	(CFG_HISTORY_ITEMS > 0)
	history_init(&rdl->history);
#endif
	return rdl;
}


char *readline(rdln_t *rdl, int c)
{
	switch (GETSTAT(rdl->state)) {
	case RDL_APPEND:
		switch (c) {
		case 0x1b:	/* ESCAPE */
		case 0x8:	/* backspace */
		case 0x7f:	/* delete */
		case 0x1:	/* Ctrl-A home */
		case 0x5:	/* Ctrl-E end */
		case 0x6:	/* Ctrl-F history */
		case 0x15:	/* Ctrl-U kill line */
		case 0x17:	/* Ctrl-W delete word */
			rdl->hold[rdl->hdlen++] = c;
			rdl->state = SETSTAT(rdl->state, RDL_HOLD);
			break;
		case '\r':
		case '\n':
			r_puts(rdl, "\r\n");
                        rdl->lbuf[rdl->idx] = 0;
			rdl->idx = rdl->cursor = 0;
#if	(CFG_HISTORY_ITEMS > 0)
			history_add(&rdl->history, rdl->lbuf);
			history_reset(&rdl->history);
#endif
			return rdl->lbuf;
		default:
			if (isprint(c)) {
				readline_insert(rdl, c);
			}
		}
		break;

	case RDL_HOLD:
		rdl->hold[rdl->hdlen++] = c;
		break;
	}
	//printf("++ %d %d %d %d\r\n", rdl->idx, rdl->cursor, rdl->hdlen, c);
	if (rdl->hdlen) {
		rdl->state = SETSTAT(rdl->state, readline_uphold(rdl));
	}
	//printf("-- %d %d %d %d\r\n", rdl->idx, rdl->cursor, rdl->hdlen, c);
	return NULL;
}

static int readline_uphold(rdln_t *rdl)
{
	int	i, n;

	for (i = n = 0; line_cmd[i].cmd; i++) {
		if (line_cmd[i].len == 1) {
			if (line_cmd[i].cmd[0] != rdl->hold[0]) {
				continue;
			} else {
				line_cmd[i].func(rdl);
				rdl->hdlen = 0;
				return RDL_APPEND;
			}
		}
		if (!memcmp(line_cmd[i].cmd, rdl->hold, rdl->hdlen)) {
			n++;
		}
		if ((rdl->hdlen == line_cmd[i].len) &&
				!memcmp(line_cmd[i].cmd, rdl->hold, line_cmd[i].len)) {
			line_cmd[i].func(rdl);
			rdl->hdlen = 0;
			return RDL_APPEND;
		}
	}
	if (n) {	/* partial match so we need more characters */
		return RDL_HOLD;
	}
	/* miss-matched pattern so we discard them */
	rdl->hdlen = 0;
	return RDL_APPEND;
}

static int readline_insert(rdln_t *rdl, int c)
{
	if (rdl->idx >= CFG_READLINE_BUFFER) {
		return 0;	/* line buffer full */
	}
	if (rdl->cursor == rdl->idx) {
		rdl->lbuf[rdl->idx] = (char) c;
		rdl->lbuf[rdl->idx+1] = 0;
		r_puts(rdl, &rdl->lbuf[rdl->idx]);
		rdl->idx++;
		rdl->cursor++;
	} else {
		memmove(&rdl->lbuf[rdl->cursor+1], &rdl->lbuf[rdl->cursor], 
				rdl->idx - rdl->cursor + 1);
		rdl->lbuf[rdl->cursor] = (char) c;
		rdl->idx++;
		readline_render(rdl, 0);
		readline_cursor_right(rdl);
	}
	return 1;
}

static int readline_render(rdln_t *rdl, int curmove)
{
	char	buf[16];

	if (curmove > 0) {		/* cursor move right */
		sprintf(buf, "\033[%dC", (unsigned char)curmove);
		r_puts(rdl, buf);
	} else if (curmove < 0) {	/* cursor move left */
		sprintf(buf, "\033[%dD", (unsigned char)(- curmove));
		r_puts(rdl, buf);
	}
	r_puts(rdl, &rdl->lbuf[rdl->cursor]);
	if (rdl->idx > rdl->cursor) {
		sprintf(buf, "\033[K\033[%dD", (unsigned char)(rdl->idx - rdl->cursor));
	} else {
		strcpy(buf, "\033[K");
	}
	r_puts(rdl, buf);
	return 0;
}

static int readline_dummy(rdln_t *rdl)
{
	return 0;
}

static int readline_backspace(rdln_t *rdl)
{
	if ((rdl->idx == 0) || (rdl->cursor == 0)) {
		return 0;	/* nothing to delete */
	}
	
	if (rdl->cursor == rdl->idx) {
		rdl->cursor--;
		rdl->idx--;
		r_puts(rdl, "\b \b");	/* delete the last char in display */
	} else {
		memmove(&rdl->lbuf[rdl->cursor-1], &rdl->lbuf[rdl->cursor],
                                rdl->idx - rdl->cursor + 1);
		rdl->cursor--;
		rdl->idx--;
		readline_render(rdl, -1);
	}
	return 1;
}


static int readline_delete(rdln_t *rdl)
{
	if ((rdl->idx == 0) || (rdl->cursor == rdl->idx)) {
		return 0;	/* nothing to delete */
	}
	
	memmove(&rdl->lbuf[rdl->cursor], &rdl->lbuf[rdl->cursor+1],
			rdl->idx - rdl->cursor + 1);
	rdl->idx--;
	return readline_render(rdl, 0);
}

static int readline_kill(rdln_t *rdl)
{
	int	c = rdl->cursor;

	if (!rdl->idx || !rdl->cursor) {
		return 0;
	}
	if (rdl->cursor == rdl->idx) {
		rdl->idx = rdl->cursor = 0;
	} else {
		memmove(rdl->lbuf, &rdl->lbuf[rdl->cursor], rdl->idx - rdl->cursor + 1);
		rdl->idx -= rdl->cursor;
		rdl->cursor = 0;
	}
	rdl->lbuf[rdl->idx] = 0;
	return readline_render(rdl, - c);
}


static int wedge(char c)
{
	if (isspace((int)((unsigned char)c)) || !c || (c == '.')) {
		return 1;
	}
	return 0;
}

static int readline_word_erase(rdln_t *rdl)
{
	int	tmp = rdl->cursor;

	if (!rdl->cursor) {
		return 0;	/* nothing to erase */
	}
	while (rdl->cursor && wedge(rdl->lbuf[rdl->cursor])) rdl->cursor--;
	while (rdl->cursor && !wedge(rdl->lbuf[rdl->cursor])) rdl->cursor--;
	if (rdl->cursor) {	/* it must move to blank space */
		rdl->cursor++;	/* so we adjust it back to word boundary */
	}
	memmove(&rdl->lbuf[rdl->cursor],  &rdl->lbuf[tmp], rdl->idx - tmp + 1);
	rdl->idx -= (tmp - rdl->cursor);
	return readline_render(rdl, rdl->cursor - tmp);
}

static int readline_cursor_left(rdln_t *rdl)
{
	if (rdl->cursor) {
		r_puts(rdl, "\033[1D");
		rdl->cursor--;
	}
	return 0;
}

static int readline_cursor_right(rdln_t *rdl)
{
	if (rdl->cursor < rdl->idx) {
		r_puts(rdl, "\033[1C");
		rdl->cursor++;
	}
	return 0;
}

static int readline_cursor_home(rdln_t *rdl)
{
	char	buf[8];

	if (rdl->cursor) {
		sprintf(buf, "\033[%dD", (unsigned char)rdl->cursor);
		r_puts(rdl, buf);
		rdl->cursor = 0;
	}
	return 0;
}

static int readline_cursor_end(rdln_t *rdl)
{
	char	buf[8];

	if (rdl->cursor < rdl->idx) {
		sprintf(buf, "\033[%dC", (unsigned char)(rdl->idx - rdl->cursor));
		r_puts(rdl, buf);
		rdl->cursor = rdl->idx;
	}
	return 0;
}

#if	(CFG_HISTORY_ITEMS > 0)
static int readline_history_up(rdln_t *rdl)
{
	hist_t	*hp = &rdl->history;
	int	n;

	if ((n = history_copy_backward(hp, rdl->lbuf, CFG_READLINE_BUFFER)) > 0) {
		rdl->idx = n - 1;	/* offset from the tailing '\0' */
		n = rdl->cursor;
		rdl->cursor = 0;
		readline_render(rdl, -n);
		readline_cursor_end(rdl);
	}
	return 0;
}

static int readline_history_down(rdln_t *rdl)
{
	hist_t	*hp = &rdl->history;
	int	n;

	if ((n = history_copy_forward(hp, rdl->lbuf, CFG_READLINE_BUFFER)) > 0) {
		rdl->idx = n - 1;
		n = rdl->cursor;
		rdl->cursor = 0;
		readline_render(rdl, -n);
		readline_cursor_end(rdl);
	}
	return 0;
}

int readline_history_dump(rdln_t *rdl)
{
	hist_t	*hp = &rdl->history;
	char	buf[CFG_READLINE_BUFFER+8];
	int	i;

	r_puts(rdl, "\r\n");
	for (i = hp->h_count - 1; i >= 0; i--) {
		sprintf(buf, "%2d", i);
		strcat(buf, "  ");
		history_copy(hp, i, buf+4, CFG_READLINE_BUFFER);
		strcat(buf, "\r\n");
		r_puts(rdl, buf);
	}
	return 0;
}


int cmd_history(void *taskarg, int argc, char **argv)
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
			cli_main(taskarg, argc, argxs);
		}
		task_puts(xtcb, "#> ");
		return 0;	
	}	
	return -2;
}

#endif	/* CFG_HISTORY_ITEMS */


#ifdef	EXECUTABLE
/* Build: gcc -Wall -DEXECUTABLE -o readline readline.c  */


#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

struct termios	newtio, oldtio;

void handle_sigint(int sig) 
{
	tcsetattr(STDIN_FILENO, TCSANOW, &oldtio);
	exit(0);
}

int main()
{
	rdln_t	rdl;
	char	*s, ch[4];
	int	flags;

    	tcgetattr(STDIN_FILENO, &oldtio);
	newtio = oldtio;
	newtio.c_lflag &= ~(ICANON | ECHO);
	newtio.c_cc[VMIN] = 1;
	newtio.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &newtio);

	signal(SIGINT, handle_sigint);

	flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

	readline_init(&rdl);
	r_puts(rdl, "PS1> ");
	while (1) {
		if (read(STDIN_FILENO, ch, 1) < 1) {
			usleep(100);
			continue;
		}
		if ((s = readline(&rdl, ch[0])) != NULL) {
			puts(s);
			r_puts(rdl, "PS1> ");
		}
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &oldtio);
	return 0;
}

#endif	/* EXECUTABLE */
