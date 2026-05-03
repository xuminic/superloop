
#ifndef _BOARD_STM32G474RE_
#define _BOARD_STM32G474RE_

/* led.c */
#define CFG_LED_NUMBER          1       /* define the number of managed LEDs */
#define CFG_LED_RINGBUF         32      /* define the size of the ring buffer */
#define CFG_LED_TICK            120     /* define the ticks for 1T: if 1 tick = 1ms, then 100ms */

/* cli.c */
#define CFG_CLI_MAX_PARAM       16      /* maximum command line parameters */

/* crc16.c */
#define CFG_CRC16_LOOKUP        1       /* require a lookup table for CRC16 (modbus) */

/* platform.c */
#define CFG_LOG_BUFF            256

/* readline.c history.c */
#define CFG_HISTORY_ITEMS       32      /* maximum history items */
#define CFG_HISTORY_POOL        ((CFG_HISTORY_ITEMS + 1) * 32)
#define CFG_READLINE_BUFFER     128     /* the size of the line buffer */

/* uart.c */
#define CFG_UART_RXBUF          64      /* the ring buffer for UART receiving */


#define CFG_PARAM_SIZE          64      /* the parameter block size in flash sectors */

#endif

