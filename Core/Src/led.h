
#ifndef	_LED_H_
#define _LED_H_

#include "board.h"

typedef	struct	{
	void	*hgpio;	/* the gpio group handler */
	int	pin;	/* the pin to drive the LED */

	int	acc;	/* Bresenham PWM accumulator: lowest priority */
	int	duty;	/* 0-100 */
	int	step;	/* step of dynamic duty */
	int	hold;	/* duty holding time: 0 = disabled */
	int	dcnt;	/* holding time counter: matching = change duty */

	unsigned code;	/* current encoded morse letter */
	int	pos;	/* current symbol position in the morse letter */
	int	ticks;	/* counting dow ticks for the current symbol */

	char	ringbuf[CFG_LED_RINGBUF+1];
	int	rblen;	/* length of content in the ring buffer */
	int	rbnow;	/* current letter in the ring buffer */

	char	*telegram;	/* higher priority than the ring buffer */
} led_t;


#ifdef __cplusplus
extern "C"
{
#endif

led_t *led_open(void *hgpio, int pin);
void led_close(led_t *led);
int led_telegram(led_t *led, char *s);
int led_ticker(led_t *led, char *s);
int led_pwm_light(led_t *led, int duty);
int led_pwm_breath(led_t *led, int step, int ticks);
int led_tick(void *tcb);
void led_init(void *hgpio, int pin);
int led_command(void *taskarg, int argc, char **argv);
void led_dump(void *taskarg, led_t *l);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _LED_H_ */
