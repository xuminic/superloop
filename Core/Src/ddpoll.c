/**
  ******************************************************************************
  * @file           : ddpoll.c
  * @brief          : the generic device driver polling system
  ******************************************************************************
  */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "main.h"
#include "ddpoll.h"


static	file_t	pollen[CFG_MAX_FILENO];


file_t *ddp_open(void *dev_id)
{
	int	i;

	for (i = 0; i < CFG_MAX_FILENO; i++) {
		if (pollen[i].handler == NULL) {
			pollen[i].handler = dev_id;
			return &pollen[i];
		}
	}
	return NULL;
}

void ddp_close(file_t *dfp)
{
	memset(dfp, 0, sizeof(file_t));
}


file_t *ddp_whoami(void *handler)
{
	int	i;

	for (i = 0; i < CFG_MAX_FILENO; i++) {
		if (pollen[i].handler == handler) {
			return &pollen[i];
		}
	}
	return NULL;
}


#if 0
file_t *ddp_polling(void)
{
	file_t	*poll = pollen;
	int	i;

	for (i = 0; i < CFG_MAX_FILENO; i++, poll++) {
		if (poll->handler == NULL) {
			continue;
		}
		if (poll->state == FSTAT_IDLE) {
			continue;
		}
		if (poll->state & (FSTAT_SENT | FSTAT_RCVD)) {
			return poll;	/* recent operation finished */
		}
		if ((poll->state & FCMD_SEND) && (poll->state & FCMD_TX_POL) && poll->f->putc) {
			if (poll->tx_len == 0) {	/* always callback */
				poll->f->putc(poll, 0);
			} else if (poll->tx_now >= poll->tx_len) {	/* transmission finished */
				poll->state |= FSTAT_SENT;
			} else {
				rc = poll->f->putc(poll, poll->tx_buf[poll->tx_now]);
				if (rc == DDERR_NONE) {
					poll->tx_now++;
				}
			}
		}
		if ((poll->state & FCMD_RECV) && (poll->state & FCMD_RX_POL) && poll->f->getc) {
			if (poll->rx_len == 0) {	/* always callback */
				poll->f->gettc(poll);
			} else if (poll->rx_now >= poll->rx_len) {	/* transmission finished */
				poll->state |= FSTAT_RCVD;
			} else if ((rc = poll->f->getc(poll)) >= 0) {	/* received a byte */
				poll->rx_buf[poll->rx_now++] = (char) rc;
				if ((rc == '\r') || (rc == '\n')) {
					poll->state |= FSTAT_CR;
					if (poll->state & FCMD_CR) {	/* return by LF/CR */
						poll->state |= FSTAT_RCVD;
					}
				}
			} else if ((rc == DDERR_BREAK) && (poll->state & FCMD_PART) && poll->rx_now) {
				poll->state |= FSTAT_RCVD;
			}
		}
	}
	return NULL;
}
#endif

