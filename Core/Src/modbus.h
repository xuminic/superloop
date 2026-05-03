
#ifndef	_MODBUS_H_
#define _MODBUS_H_

#define	CFG_MODBUS_BUFSIZE	256+16

#define MBUS_STAT_READY		0	/* received a full modbus package */
#define MBUS_STAT_RESET		-1	/* discard current data and restart */
#define MBUS_STAT_EXIT		-2	/* exit the modbus transmitter */
#define MBUS_STAT_CRC		-3	/* CRC error */

enum	{
	MREG_SLAVE_ID,
	MREG_COMMAND,
	MREG_PWM_FREQ,
	MREG_PWM_DTIME,		/* dead time */
	MREG_PWM_DUTY,		/* default duty */
	MREG_PWM_DUTY2,		/* second duty */
	MREG_MAX
};


typedef	struct	{
	char	buf[CFG_MODBUS_BUFSIZE];
	int	idx;	/* current receiving position in the buffer */
	int	want;	/* expected bytes for the current transaction */

	int	stat_rx;	/* received modbus packets */
	int	stat_tx;	/* responded packets */
} mbus_t;

#ifdef __cplusplus
extern "C"
{
#endif

int modbus_receive(void *taskarg, mbus_t *mb, int ch);
int modbus_main(void *taskarg, mbus_t *mb);
void modbus_reset(mbus_t *mb);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _MODBUS_H_ */
