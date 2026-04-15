
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

#include "platform.h"


#define CFG_RDLN_BUFFER		128
#define CFG_RDLN_HISTORY	32
#define CFG_RDLN_HPOOL		(CFG_RDLN_HISTORY * 32)

#define RDL_APPEND		0
#define RDL_HOLD		1
#define RDL_STATMSK		0xf
#define RDL_REPLACE		0x10	/* 0: insert mode 1: replace mode */


typedef	struct	{
	char	lbuf[CFG_RDLN_BUFFER];
	int	idx;
	int	cursor;
	int	state;
	char	hold[8];
	int	hdlen;
#if	CFG_RDLN_HISTORY > 0
	char	*hist[CFG_RDLN_HISTORY];
	int	hnow;
	char	hpool[CFG_RDLN_HPOOL];
#endif
} rdln_t;

static	rdln_t	line;

static int readline_uphold(rdln_t *rdl);
static int readline_insert(rdln_t *rdl, int c);
static int readline_render(rdln_t *rdl, int curmove);
static int readline_dummy(rdln_t *rdl);
static int readline_backspace(rdln_t *rdl);
static int readline_delete(rdln_t *rdl);
static int readline_cursor_left(rdln_t *rdl);
static int readline_cursor_right(rdln_t *rdl);
static int readline_cursor_home(rdln_t *rdl);
static int readline_cursor_end(rdln_t *rdl);
static int readline_kill(rdln_t *rdl);
	
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
	{ "\x17",	1,	readline_dummy },	/* Ctrl-W */
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
	{ "\x1b[5~",	4,	readline_dummy }, 	/* PageUp */
	{ "\x1b[6~",	4,	readline_dummy }, 	/* PageDown */
	{ 0, 0, NULL }
};

#define CURSOR_LEFT()	u_puts("\033[1D")
#define CURSOR_RIGHT()	u_puts("\033[1C")
#define ERASE_TO_END()	u_puts("\033[K")  // 清除从当前光标到行尾的所有内容

void *readline_init(void)
{
	memset(&line, 0, sizeof(rdln_t));
	return &line;
}

char *readline(int c)
{
	switch (line.state) {
	case RDL_APPEND:
		switch (c) {
		case 0x1b:	/* ESCAPE */
		case 0x8:	/* backspace */
		case 0x7f:	/* delete */
		case 0x1:	/* Ctrl-A home */
		case 0x5:	/* Ctrl-E end */
		case 0x15:	/* Ctrl-U kill line */
		case 0x17:	/* Ctrl-W delete word */
			line.hold[line.hdlen++] = c;
			line.state = RDL_HOLD;
			break;
		case '\r':
		case '\n':
			u_puts("\r\n");
                        line.lbuf[line.idx] = 0;
			line.idx = line.cursor = 0;
			return line.lbuf;
		default:
			if (isprint(c)) {
				readline_insert(&line, c);
			}
		}
		break;

	case RDL_HOLD:
		line.hold[line.hdlen++] = c;
		break;
	}
	//printf("-> %d %d %d %d\n", line.idx, line.cursor, line.hdlen, c);
	if (line.hdlen) {
		line.state = readline_uphold(&line);
	}
	return NULL;
}

static int readline_uphold(rdln_t *rdl)
{
	int	i, n;

	for (i = n = 0; line_cmd[i].cmd; i++) {
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
	if (rdl->idx >= CFG_RDLN_BUFFER) {
		return 0;	/* line buffer full */
	}
	if (rdl->cursor == rdl->idx) {
		rdl->lbuf[rdl->idx++] = (char) c;
		rdl->lbuf[rdl->idx] = 0;
	} else {
		memmove(&rdl->lbuf[rdl->cursor+1], &rdl->lbuf[rdl->cursor], 
				rdl->idx - rdl->cursor + 1);
		rdl->lbuf[rdl->cursor] = (char) c;
		rdl->idx++;
	}
	readline_render(rdl, 0);
	rdl->cursor++;
	CURSOR_RIGHT();
	return 1;
}

static int readline_render(rdln_t *rdl, int curmove)
{
	char	buf[24];

	if (curmove > 0) {		/* cursor move right */
		sprintf(buf, "\033[%dC", curmove);
		u_puts(buf);
	} else if (curmove < 0) {	/* cursor move left */
		sprintf(buf, "\033[%dD", - curmove);
		u_puts(buf);
	}
	u_puts(&rdl->lbuf[rdl->cursor]);
	if (rdl->idx > rdl->cursor) {
		sprintf(buf, "\033[K\033[%dD", rdl->idx - rdl->cursor);
	} else {
		strcpy(buf, "\033[K");
	}
	u_puts(buf);
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
		u_puts("\b \b");	/* delete the last char in display */
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

static int readline_cursor_left(rdln_t *rdl)
{
	if (rdl->cursor) {
		CURSOR_LEFT();
		rdl->cursor--;
	}
	return 0;
}

static int readline_cursor_right(rdln_t *rdl)
{
	if (rdl->cursor < rdl->idx) {
		CURSOR_RIGHT();
		rdl->cursor++;
	}
	return 0;
}

static int readline_cursor_home(rdln_t *rdl)
{
	while (rdl->cursor) {
		CURSOR_LEFT();
		rdl->cursor--;
	}
	return 0;
}

static int readline_cursor_end(rdln_t *rdl)
{
	while (rdl->cursor < rdl->idx) {
		CURSOR_RIGHT();
		rdl->cursor++;
	}
	return 0;
}


#ifdef	EXECUTABLE
/* Build: gcc -Wall -DEXECUTABLE -o readline readline.c  */


#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

struct termios	newtio, oldtio;

void handle_sigint(int sig) 
{
	tcsetattr(STDIN_FILENO, TCSANOW, &oldtio);
	exit(0);
}

int u_puts(char *s)
{
	return write(STDOUT_FILENO, s, strlen(s));
}

int main()
{
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

	readline_init();
	u_puts("PS1> ");
	while (1) {
		if (read(STDIN_FILENO, ch, 1) < 1) {
			usleep(100);
			continue;
		}
		if ((s = readline(ch[0])) != NULL) {
			puts(s);
			u_puts("PS1> ");
		}
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &oldtio);
	return 0;
}

#endif	/* EXECUTABLE */
