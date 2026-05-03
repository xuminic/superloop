

#ifndef	_READLINE_H_
#define _READLINE_H_

#include "board.h"

typedef	struct	{
	int	off;  /* the offset in the pool */
	int	len;  /* the string length includes '\0' */
} item_t;

typedef	struct	{
	item_t	hist[CFG_HISTORY_ITEMS];
	int	h_head;   /* the oldest message */
	int	h_tail;   /* the slot for the next message */
	int	h_count;  /* the number of stored messages */
	int	h_cur;
	int	h_stat;

	char	pool[CFG_HISTORY_POOL];
	int	p_head;   /* pool read - the start point of the oldest data */
	int	p_tail;   /* pool write - the next position for writing */
} hist_t;


#define READLINE_STAT_REPLACE	1	/* 0: insert */
#define READLINE_STAT_CHANGED	2	/* current line is changed */


typedef	struct	{
	void	*uart;		/* point to the i/o of character stream */
	char	lbuf[CFG_READLINE_BUFFER];
	int	idx;
	int	cursor;
	int	state;
	char	hold[8];
	int	hdlen;
#if	(CFG_HISTORY_ITEMS > 0)
	hist_t	history;
#endif
} rdln_t;


#ifdef __cplusplus
extern "C"
{
#endif

/* readline.c */
void *readline_init(rdln_t *rdl, void *uart);
char *readline(rdln_t *rdl, int c);
#if	(CFG_HISTORY_ITEMS > 0)
int readline_history_dump(rdln_t *rdl);
int cmd_history(void *taskarg, int argc, char **argv);
#endif

/* history.c */
void history_init(hist_t *hp);
char *history_add(hist_t *hp, char *s);
int history_copy(hist_t *hp, int n, char *buf, int blen);
int history_number(hist_t *hp);
int history_copy_backward(hist_t *hp, char *buf, int blen);
int history_copy_forward(hist_t *hp, char *buf, int blen);
void history_reset(hist_t *hp);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _READLINE_H_ */

