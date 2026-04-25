
#include <ctype.h>
#include <stdio.h>
#include <string.h>

extern	int u_puts(char *s);

static  const   char    hex_tab[] = "0123456789ABCDEF";
static	char	logbuf[84];

/* 00000000-  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................\r\n
   index:  0         11 (Hex)                              61 (ASCII) */
void hexdump(char *s, int len) 
{
	char	*bp;
	int 	i, n;

	bp = (char*)(((unsigned long) s) & ~0xf);
	while (bp < s + len) {
        	/* initialize the template */
        	memset(logbuf, ' ', sizeof(logbuf));
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
		u_puts(logbuf);
	}
}

