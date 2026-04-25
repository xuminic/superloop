

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "readline.h"


static int history_compare(hist_t *hp, char *s);
static int history_pool_used(hist_t *hp);
static int history_pool_free(hist_t *hp);
static void history_del(hist_t *hp);
static int history_index(hist_t *hp, int n);

void history_init(hist_t *hp)
{
	memset(hp, 0, sizeof(hist_t));
}

char *history_add(hist_t *hp, char *s)
{
	int	off, first, len = strlen(s) + 1;

	if ((len == 1) || (len > CFG_HISTORY_POOL)) {
		return NULL;	/* don't save the empty lines */
	}

	if (!history_compare(hp, s)) {
		return NULL;	/* drop the same line */
	}

	if (ispunct((unsigned char)*s)) {	/* don't save string starts with punctures */
		return NULL;	/* especially for '!' command */
	}

	/* if hist list full, delete the oldest */
	if (hp->h_count == CFG_HISTORY_ITEMS) {
		history_del(hp);
	}

	/* if not enough space in pool, keep deleting till good space */
	while (history_pool_free(hp) < len) {
		history_del(hp);
	}

	off = hp->p_tail;

	/* write into the pool (possible with wrap) */
	first = CFG_HISTORY_POOL - hp->p_tail;

	if (first >= len) {
		memcpy(&hp->pool[hp->p_tail], s, len);
	} else {
		memcpy(&hp->pool[hp->p_tail], s, first);
		memcpy(hp->pool, s + first, len - first);
	}

	/* update the tail point */
	hp->p_tail = (hp->p_tail + len) % CFG_HISTORY_POOL;

	/* update the hist list */
	hp->hist[hp->h_tail].off = off;
	hp->hist[hp->h_tail].len = len;

#if	(CFG_HISTORY_ITEMS > 0)
	hp->h_tail = (hp->h_tail + 1) % CFG_HISTORY_ITEMS;
#endif
	hp->h_count++;

	return &hp->pool[off];  /* note: string might wrap */
}

int history_copy(hist_t *hp, int n, char *buf, int blen)
{
	item_t	*h;
	int	i, first;

	if ((i = history_index(hp, n)) < 0) {
		return -1;
	}

	h = &hp->hist[i];
	if (h->len > blen) {
		return -2;
	}

	first = CFG_HISTORY_POOL - h->off;

	if (first >= h->len) {
		memcpy(buf, &hp->pool[h->off], h->len);
	} else {
		memcpy(buf, &hp->pool[h->off], first);
		memcpy(buf + first, hp->pool, h->len - first);
	}
	return h->len;
}


int history_number(hist_t *hp)
{
	return hp->h_count;
}

int history_copy_backward(hist_t *hp, char *buf, int blen)
{
	if ((hp->h_count == 0) || (hp->h_cur == hp->h_count - 1)) {
		return 0;
	}
	return history_copy(hp, ++hp->h_cur, buf, blen);
}

int history_copy_forward(hist_t *hp, char *buf, int blen)
{
	if (hp->h_count == 0) {
		return 0;
	}
	if (hp->h_cur == 0) {
		buf[0] = 0;
		hp->h_cur--;
		return 1;
	}
	return history_copy(hp, --hp->h_cur, buf, blen);
}

void history_reset(hist_t *hp)
{
	hp->h_cur = -1;
	hp->h_stat = 0;
}

static int history_compare(hist_t *hp, char *s)
{
	item_t	*h;
	int	i, first;

	if ((i = hp->h_tail - 1) < 0) {
		i = CFG_HISTORY_ITEMS - 1;
	}
	h = &hp->hist[i];
	if (h->len != (strlen(s) + 1)) {
		return 1;
	}

	first = CFG_HISTORY_POOL - h->off;
	if (first >= h->len) {
		return strcmp(&hp->pool[h->off], s);
	}
	if (strncmp(&hp->pool[h->off], s, first)) {
		return 2;
	}
	return strncmp(s + first, hp->pool, h->len - first);
}


static int history_pool_used(hist_t *hp)
{
	if (hp->p_tail >= hp->p_head) {
		return hp->p_tail - hp->p_head;
	}
	return CFG_HISTORY_POOL - (hp->p_head - hp->p_tail);
}

static int history_pool_free(hist_t *hp)
{
	return CFG_HISTORY_POOL - history_pool_used(hp);
}


static void history_del(hist_t *hp)
{
	item_t	*h = &hp->hist[hp->h_head];

	if (hp->h_count) {
		/* move the pool head forward */
		hp->p_head = (h->off + h->len) % CFG_HISTORY_POOL;

		/* move the hist list forward */
#if	(CFG_HISTORY_ITEMS > 0)
		hp->h_head = (hp->h_head + 1) % CFG_HISTORY_ITEMS;
#endif
		hp->h_count--;
	}
}

static int history_index(hist_t *hp, int n)
{
	if (n < 0 || n >= hp->h_count) {
		return -1;
	}
#if	(CFG_HISTORY_ITEMS > 0)
	return (hp->h_tail + CFG_HISTORY_ITEMS - n - 1) % CFG_HISTORY_ITEMS;
#else
	return 0;
#endif
}

#if 0
static char *history_get(hist_t *hp, int n)
{
	int	i;

	if ((i = history_index(hp, n)) < 0) {
		return NULL;
	}
	return &hp->pool[hp->hist[i].off];	/* note: string might wrap */
}
#endif


#ifdef	EXECUTABLE
/* Build: gcc -Wall -DEXECUTABLE -o readline readline.c  */

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>


#endif	/* EXECUTABLE */
