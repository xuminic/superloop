
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef	EXECUTABLE
#include "main.h"
#endif
#include "platform.h"
#include "board.h"
#include "led.h"


#define MOREARG(c,v)    {       \
        --(c), ++(v); \
        if (((c) == 0) || (**(v) == '-') || (**(v) == '+')) { \
                fprintf(stderr, "missing parameters\n"); \
                return -1; \
        }\
}


static	const	uint32_t	morse32[128] = {
	0xFFFFFFF0,0xFFF12111,0xD1111111,0xFFF21212,0xFFF12121,0xF2121112,0xD1211212,0xFF212111,
	0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,
	0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,
	0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,
	0xFFFFFFF0,0xFF221212,0xFF121121,0xFFFFFFF0,0xF2112111,0xFFFFFFF0,0xFFF11121,0xFF122221,
	0xFFF12212,0xFF212212,0xFFFFFFF0,0xFFF12121,0xFF221122,0xFF211112,0xFF212121,0xFFF12112,
	0xFFF22222,0xFFF22221,0xFFF22211,0xFFF22111,0xFFF21111,0xFFF11111,0xFFF11112,0xFFF11122,
	0xFFF11222,0xFFF12222,0xFF111222,0xFF121212,0xFFFFFFF0,0xFFF21112,0xFFFFFFF0,0xFF112211,
	0xFF121221,0xFFFFFF21,0xFFFF1112,0xFFFF1212,0xFFFFF112,0xFFFFFFF1,0xFFFF1211,0xFFFFF122,
	0xFFFF1111,0xFFFFFF11,0xFFFF2221,0xFFFFF212,0xFFFF1121,0xFFFFFF22,0xFFFFFF12,0xFFFFF222,
	0xFFFF1221,0xFFFF2122,0xFFFFF121,0xFFFFF111,0xFFFFFFF2,0xFFFFF211,0xFFFF2111,0xFFFFF221,
	0xFFFF2112,0xFFFF2212,0xFFFF1122,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFF212211,
	0xFFFFFFF0,0xFFFFFF21,0xFFFF1112,0xFFFF1212,0xFFFFF112,0xFFFFFFF1,0xFFFF1211,0xFFFFF122,
	0xFFFF1111,0xFFFFFF11,0xFFFF2221,0xFFFFF212,0xFFFF1121,0xFFFFFF22,0xFFFFFF12,0xFFFFF222,
	0xFFFF1221,0xFFFF2122,0xFFFFF121,0xFFFFF111,0xFFFFFFF2,0xFFFFF211,0xFFFF2111,0xFFFFF221,
	0xFFFF2112,0xFFFF2212,0xFFFF1122,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0,0xFFFFFFF0
};


static	led_t	ledtab[CFG_LED_NUMBER];

static void led_morse(led_t *m);
static void led_reload(led_t *m);
static void led_pwm(led_t *l);


led_t *led_open(void *hgpio, int pin)
{
	int	i;

	for (i = 0; i < CFG_LED_NUMBER; i++) {
		if (ledtab[i].hgpio == NULL) {
			memset(&ledtab[i], 0, sizeof(led_t));
			ledtab[i].hgpio = hgpio;
			ledtab[i].pin = pin;
			return &ledtab[i];
		}
	}
	return NULL;
}

void led_close(led_t *led)
{
	memset(led, 0, sizeof(led_t));
}

int led_telegram(led_t *led, char *s)
{
	led->telegram = s;
	led_reload(led);
	return s == NULL ? 0 : strlen(s);
}

int led_ticker(led_t *led, char *s)
{
	if (!s || !*s) {
		led->rblen = 0;
	} else {
		strncpy(led->ringbuf, s, CFG_LED_RINGBUF);
		led->ringbuf[CFG_LED_RINGBUF] = 0;
		led->rblen = strlen(led->ringbuf);
	}
	led_reload(led);
	return led->rblen;
}

int led_pwm_light(led_t *led, int duty)
{
	led->acc  = 0;
	led->duty = duty > 100 ? 100 : duty;
	return led->duty;
}

int led_pwm_breath(led_t *led, int step, int hold)
{
	led->acc  = led->duty = 0;
	led->step = step > 50 ? 50 : step;
	led->hold = hold;
	led->dcnt = hold;
	return 0;
}

int led_tick(void *tcb)
{
	int	i;

	for (i = 0; i < CFG_LED_NUMBER; i++) {
		if (ledtab[i].hgpio == NULL) {
			continue;
		}
		if (ledtab[i].telegram || ledtab[i].rblen) {
			led_morse(&ledtab[i]);
		} else if (ledtab[i].duty || ledtab[i].hold) {
			led_pwm(&ledtab[i]);
		}
	}
	return 0;
}

static void led_morse(led_t *m)
{
	uint8_t		state;

	if (m->ticks) {
		m->ticks--;
		return;
	}

	state = (m->code >> (m->pos * 2)) & 0x3;
	//printf("S%d\n", state);

	switch (state) {
	case 0:		/* 00 = OFF 1T */
		bai_led_off(m->hgpio, m->pin);
		m->ticks = CFG_LED_TICK;
		break;

	case 1: 	/* 01 = DOT 1T */
		bai_led_on(m->hgpio, m->pin);
		m->ticks = CFG_LED_TICK;
		break;

	case 2: 	/* 10 = DASH 3T */
		bai_led_on(m->hgpio, m->pin);
		m->ticks = 3 * CFG_LED_TICK;
		break;

	case 3: 	/* 11 = END = OFF 2T */
		bai_led_off(m->hgpio, m->pin);
		m->ticks = 2 * CFG_LED_TICK;

		/* loading next letter */
		led_reload(m);
		return;
	}
	m->pos++;
}

static void led_reload(led_t *m)
{
	char	c;

	if (!m->telegram) {	/* load from the ring buffer */
		c = m->ringbuf[m->rbnow++];
	} else if (*m->telegram) {	/* load from the telegram */
		c = *m->telegram++;
	} else {	/* turn off telegram */
		m->telegram = NULL;
		c = m->ringbuf[m->rbnow++];
	}
	if (m->rbnow == m->rblen) {
		m->rbnow = 0;
	}
	m->code = morse32[(uint8_t)c];
	m->pos = 0;
	//printf("L:%c\n", c);
}

static void led_pwm(led_t *l)
{
	if (l->hold && l->step) {	/* breath mode */
		l->dcnt--;
		if (l->dcnt <= 0) {
			l->dcnt = l->hold;
			l->duty += l->step;
			if (l->duty >= 100) {
				l->duty = 100;
				l->step = - l->step;
			}
			if (l->duty <= 0) {
				l->duty = 0;
				l->step = - l->step;
			}
		}
	}

	l->acc += l->duty;

	if (l->acc >= 100) {	/* Bresenham method: plus 100 */
		l->acc -= 100;
		bai_led_on(l->hgpio, l->pin);
	} else {
		bai_led_off(l->hgpio, l->pin);
	}
}


static	char	*led_help = "\
Usage: led [OPTION] [message]\r\n\
OPTION:\r\n\
  -l, --led NUM         specify the current LED\r\n\
  -b, --bright NUM      the brightness of the LED (%)\r\n\
  -h, --hold NUM        the hold time of each step in Breath mode (ms)\r\n\
  -s, --step NUM        the step of the brightness in Breath mode (%)\r\n\
  -m, --message         sending a one-off message\r\n\
  -t, --ticker          set the content of the rolling ticker\r\n\
  -d, --dump            dump the LED status\r\n\
";

int led_command(void *taskarg, int argc, char **argv)
{
	xtcb_t	*xtcb = taskarg;
	led_t	*led = &ledtab[0];
	int	i, duty, step, hold, todo = 0;

	duty = step = hold = -1;
	while (--argc && ((**++argv == '-') || (**argv == '+'))) {
		if (!strcmp(*argv, "-H") || !strcmp(*argv, "--help")) {
			task_puts(taskarg, led_help);
			return 0;
		} else if (!strcmp(*argv, "-l") || !strcmp(*argv, "--led")) {
			MOREARG(argc, argv);
			i = (int) strtol(*argv, NULL, 0);
			if ((i >= 0) && (i < CFG_LED_NUMBER)) {
				led = &ledtab[i];
			}
		} else if (!strcmp(*argv, "-b") || !strcmp(*argv, "--bright")) {
			MOREARG(argc, argv);
			duty = (int) strtol(*argv, NULL, 0);
		} else if (!strcmp(*argv, "-s") || !strcmp(*argv, "--step")) {
			MOREARG(argc, argv);
			step = (int) strtol(*argv, NULL, 0);
		} else if (!strcmp(*argv, "-h") || !strcmp(*argv, "--hold")) {
			MOREARG(argc, argv);
			hold = (int) strtol(*argv, NULL, 0);
		} else if (!strcmp(*argv, "-m") || !strcmp(*argv, "--message")) {
			todo = 'm';
		} else if (!strcmp(*argv, "-t") || !strcmp(*argv, "--ticker")) {
			todo = 't';
		} else if (!strcmp(*argv, "-d") || !strcmp(*argv, "--dump")) {
			todo = 'd';
		} else {
			task_printf(taskarg, "%s: unknown parameter.\r\n", *argv);
			return -1;
		}
	}

	if (led->hgpio == NULL) {
		task_puts(taskarg, "LED device not defined\r\n");
		return -2;
	}
	
	if ((duty >= 0) && (duty <= 100)) {
		led_pwm_light(led, duty);
	}
	if (step > 0) {
		step = (step > 50) ? 50 : step;
		hold = (hold < 0) ? 100 : hold;
		led_pwm_breath(led, step, hold);
	}

	if (argc) {
		strncpy(xtcb->logbuf, *argv, CFG_LOG_BUFF - 1);
		xtcb->logbuf[CFG_LOG_BUFF - 1] = 0;
	} else {
		strcpy(xtcb->logbuf, "HelloWorld");
	}
	if (todo == 'm') {
		led_telegram(led, xtcb->logbuf);
	} else if (todo == 't') {
		led_ticker(led, xtcb->logbuf);
	} else if (todo == 'd') {
		for (i = 0; i < CFG_LED_NUMBER; i++) {
			if (ledtab[i].hgpio) {
				led_dump(taskarg, &ledtab[i]);
			}
		}
	}
	return 0;
}

void led_dump(void *taskarg, led_t *l)
{
	task_printf(taskarg, "LED PWM duty:           %d\r\n", l->duty);
	task_printf(taskarg, "LED PWM Breath step:    %d\r\n", l->step);
	task_printf(taskarg, "LED PWM Breath hold:    %d\r\n", l->hold);
	task_printf(taskarg, "LED PWM Breath counter: %d\r\n", l->dcnt);
	if (l->rblen) {
		task_printf(taskarg, "Ticker Message:         <%s> (%d)\r\n", l->ringbuf, l->rbnow);
	} else {
		task_puts(taskarg, "Ticker Message:         <>\r\n");
	}
	if (l->telegram) {
		task_printf(taskarg, "Telegram Message:       <%s>\r\n", l->telegram);
	}
	task_puts(taskarg, "\r\n");
}

void led_init(void *hgpio, int pin)
{
	led_t	*l;

	if ((l = led_open(hgpio, pin)) != NULL) {
		l->duty = 50;
		l->step = 10;
		l->hold = 100;
	}
}


#ifdef	EXECUTABLE
#include <unistd.h>
#include <time.h>

/* https://www.itu.int/dms_pubrec/itu-r/rec/m/R-REC-M.1677-1-200910-I!!PDF-E.pdf */
static	const	struct {
	uint8_t	ch;
	char	*code;
} morse_table[] = {
	// ===== Misc/Prosigns =====
	{1, "...-."},     // UNDERSTOOD (SN)
	{2, "........"},  // ERROR
	{3, "-.-.-"},     // CT (start)
	{4, ".-.-."},     // AR (end)
	{5, "-...-.-"},   // BK (break)
	{6, "-.-..-.."},  // CL (going off air)
	{7, "...-.-"},    // SK (end of contact)

	{'0', "-----"}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"},
	{'4', "....-"}, {'5', "....."}, {'6', "-...."}, {'7', "--..."},
	{'8', "---.."}, {'9', "----."},

	{'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},
	{'E', "."}, {'F', "..-."}, {'G', "--."}, {'H', "...."},
	{'I', ".."}, {'J', ".---"}, {'K', "-.-"}, {'L', ".-.."},
	{'M', "--"}, {'N', "-."}, {'O', "---"}, {'P', ".--."},
	{'Q', "--.-"}, {'R', ".-."}, {'S', "..."}, {'T', "-"},
	{'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"},
	{'Y', "-.--"}, {'Z', "--.."},

	{'a', ".-"}, {'b', "-..."}, {'c', "-.-."}, {'d', "-.."},
	{'e', "."}, {'f', "..-."}, {'g', "--."}, {'h', "...."},
	{'i', ".."}, {'j', ".---"}, {'k', "-.-"}, {'l', ".-.."},
	{'m', "--"}, {'n', "-."}, {'o', "---"}, {'p', ".--."},
	{'q', "--.-"}, {'r', ".-."}, {'s', "..."}, {'t', "-"},
	{'u', "..-"}, {'v', "...-"}, {'w', ".--"}, {'x', "-..-"},
	{'y', "-.--"}, {'z', "--.."},

	// ===== Punctuation marks (ITU-R M.1677-1) =====
	{'.', ".-.-.-"},   // full stop
	{',', "--..--"},   // comma
	{'?', "..--.."},   // question mark
	{'\'', ".----."},  // apostrophe
	{'!', "-.-.--"},   // exclamation mark
	{'/', "-..-."},    // slash
	{'(', "-.--."},    // open parenthesis
	{')', "-.--.-"},   // close parenthesis
	{'&', ".-..."},    // ampersand
	{':', "---..."},   // colon
	{';', "-.-.-."},   // semicolon
	{'=', "-...-"},    // equals
	{'+', ".-.-."},    // plus
	{'-', "-....-"},   // hyphen
	{'_', "..--.-"},   // underscore
	{'"', ".-..-."},   // quotation mark
	{'$', "...-..-"},  // dollar
	{'@', ".--.-."},   // at

	// ===== space for word gap =====
	{' ', ""},
	{'\t', ""},

	{0, NULL}
};


/* my strategy: 32-bit integer has 16 encode unit, each unit present
 *  00: gap between symbol = LED off for 1T
 *  01: dot  (.) LED on for 1T
 *  10: dash (-) LED on for 3T
 *  11: end of letter = LED off for 2T
 * Lowest unit present the first dot/dash symbol.
 * ITU: The space between two letters is equal to three dots.
 * inter-symbol gap is 1T so plus 2T for end of letter
 */
static uint32_t encode_char(const char *s)
{
	uint32_t out = 0;
	int pos = 0;

	while (*s && pos < 15) {
		/* dot / dash */
		if (*s == '.') {
			out |= (1 << (pos * 2));   /* 01 */
		} else {
			out |= (2 << (pos * 2));   /* 10 */
		}
		pos++;

		/* inter-symbol gap = 00 */
		if (pos < 15) {
			out |= (0 << (pos * 2));
			pos++;
		}
		s++;
	}

	/* fill end of character 11 to the rest of bitmap */
	while (pos < 16) {
		out |= (3u << (pos * 2));
		pos++;
	}
	return out;
}

/* ITU: The space between two words is equal to seven dots.
 *      The gap from the previous letter =  3T
 *      The silence in whitespace (' ') = 2T
 *      The end of letter = 2T
 *      So the whitespace will be mapped to "... 11 11 00 00"
 *      So the whitespace is .... 1111 1111 1111 0000 = 0xfffffff0
 */
static void gen_morse_table(void)
{
	uint32_t table[128];
	int	i;

	for (i = 0; i < 128; table[i++] = 0xfffffff0);
	for (i = 0; morse_table[i].ch; i++) {
		if (*morse_table[i].code) {
			table[morse_table[i].ch] = encode_char(morse_table[i].code);
		}
	}

	printf("const uint32_t morse32[128] = {\n\t");
	for (i = 0; i < 127; i++) {
		printf("0x%08X,", table[i]);
		if ((i & 7) == 7) printf("\n\t");
	}
	printf("0x%08X\n};\n", table[i]);
}

int main(int argc, char **argv)
{
	struct	timespec ts;
	
	if (argc < 2) {
		gen_morse_table();
		return 0;
	}

	led_init(1);
	led_command(argc, argv);

	ts.tv_sec = 0;
	ts.tv_nsec = 1 * 1000 * 1000; /* 1ms */
	while (1) {
                led_tick(NULL);
		nanosleep(&ts, NULL);
	}
	return 0;
}
#endif

