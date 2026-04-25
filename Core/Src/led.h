
#ifndef	_LED_H_
#define _LED_H_

#define CFG_LED_NUMBER		1	/* define the number of managed LEDs */
#define CFG_LED_RINGBUF		16	/* define the size of the ring buffer */
#define CFG_LED_TICK		100	/* define the ticks for 1T: if 1 tick = 1ms, then 100ms */


typedef	struct	{
	int	gpio;	/* the gpio to drive the LED */

	int	acc;	/* Bresenham PWM accumulator: lowest priority */
	int	duty;	/* 0-100 */
	int	step;	/* step of dynamic duty */
	int	hold;	/* duty holding time: 0 = disabled */
	int	dcnt;	/* holding time counter: matching = change duty */

	unsigned code;	/* current encoded morse letter */
	int	pos;	/* current symbol position in the morse letter */
	int	ticks;	/* counting dow ticks for the current symbol */

	char	ringbuf[CFG_LED_RINGBUF];
	int	rblen;	/* length of content in the ring buffer */
	int	rbnow;	/* current letter in the ring buffer */

	char	*telegram;	/* higher priority than the ring buffer */
} led_t;


#ifdef __cplusplus
extern "C"
{
#endif
led_t *led_open(int gpio);
void led_close(led_t *led);
int led_telegram(led_t *led, char *s);
int led_ticker(led_t *led, char *s);
int led_pwm_light(led_t *led, int duty);
int led_pwm_breath(led_t *led, int step, int ticks);
int led_tick(void *tcb);
void led_init(int gpio);
int led_command(int argc, char **argv);
#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _LED_H_ */
