
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"

static  const   char    hex_tab[] = "0123456789ABCDEF";
static	char	*hex_last = NULL;

/* 00000000-  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................\r\n
   index:  0         11 (Hex)                              61 (ASCII) */
void hexdump(xtcb_t *xtcb, char *s, int len) 
{
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


int cmd_dump(void *taskarg, int argc, char **argv)
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
	

