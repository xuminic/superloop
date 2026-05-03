#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "platform.h"
#include "modbus.h"


uint16_t	mregs[MREG_MAX];


static int modbus_read_input(void *taskarg, mbus_t *mb);
static int modbus_write_single(void *taskarg, mbus_t *mb);
static void modbus_faulty(void *taskarg, mbus_t *mb, int code);
static void modbus_pack_send(void *taskarg, mbus_t *mb, int n);
static int modbus_frame_length(mbus_t *mb);
static uint16_t mshort(void *buf);
static void sshort(void *buf, uint16_t n);
static void spack(void *buf, uint16_t *val, int n);


void modbus_reset(mbus_t *mb)
{
	memset(mb->buf, 0, CFG_MODBUS_BUFSIZE-16);
	mb->idx = 0;
	mb->want = 0;
}

int modbus_receive(void *taskarg, mbus_t *mb, int ch)
{
	mb->buf[mb->idx++] = (char) ch;
	if (mb->want) {	/* frame length confirmed */
		if (mb->idx >= mb->want) {  /* frame completed */
			if (modbus_crc16(mb->buf, mb->want) == 0) {
				mb->idx = 0;	/* reset with buffer full */
				return MBUS_STAT_READY;
			}
			modbus_reset(mb);
			return MBUS_STAT_CRC;		/* data CRC error */
		}
	} else if ((ch = modbus_frame_length(mb)) > 0) {
		mb->want = ch;	/* confirmed the expect length */
	} else if (ch < 0) {	/* either MBUS_STAT_EXIT or MBUS_STAT_RESET */
		modbus_reset(mb);
		return ch;
	}
	return mb->idx;	/* need more data */
}

int modbus_main(void *taskarg, mbus_t *mb)
{
	mb->stat_rx++;	/* counting any received packet */

	/* save a recent copy in the same buffer - volatile area */	
	if (mb->idx < CFG_MODBUS_BUFSIZE - 20) {
		memcpy(&mb->buf[CFG_MODBUS_BUFSIZE-16], mb->buf, 16);
	}
	switch (mb->buf[1]) {
	case 0x04:	/* Read Input Registers */
		modbus_read_input(taskarg, mb);
		break;

	case 0x06:	/* Write Single Register */
		modbus_write_single(taskarg, mb);
		break;

	case 0x10:      /* Write Multiple Registers */
	case 0x03:	/* Read Holding Registers */
	default:
		modbus_faulty(taskarg, mb, 1);	/* Illegal Function */
		break;
	}
	modbus_reset(mb);
	return 0;
}


static int modbus_read_input(void *taskarg, mbus_t *mb)
{
	int	n, reg, quan;

	reg  = (int)mshort(&mb->buf[2]);
	quan = (int)mshort(&mb->buf[4]);

	switch (reg) {
	case 0:
		if (quan != 1) {
			modbus_faulty(taskarg, mb, 9);	/* illegal data range */
			break;
		}
		mb->buf[2] = 2;	/* first two bytes are unchanged */
		sshort(&mb->buf[3], mregs[MREG_SLAVE_ID]);
		modbus_pack_send(taskarg, mb, 7);
		break;

	case 0x100:
	case 0x102:
		n = quan << 1;
		if ((reg + n) > 0x104) {
			modbus_faulty(taskarg, mb, 9);	/* illegal data range */
			break;
		}
		mb->buf[2] = n;
		reg = reg - 0x100 + MREG_SLAVE_ID;
		spack(&mb->buf[3], &mregs[reg], quan);
		modbus_pack_send(taskarg, mb, 5 + n);
		break;

	case 0x1000:
	case 0x1002:
	case 0x1004:
	case 0x1006:
		n = quan << 1;
		if ((reg + n) > 0x1008) {
			modbus_faulty(taskarg, mb, 9);	/* illegal data range */
			break;
		}
		mb->buf[2] = n;
		reg = reg - 0x1000 + MREG_PWM_FREQ;
		spack(&mb->buf[3], &mregs[reg], quan);
		modbus_pack_send(taskarg, mb, 5 + n);
		break;

	default:
		modbus_faulty(taskarg, mb, 2);	/* illegal data address */
		break;
	}
	return 0;
}


static int modbus_write_single(void *taskarg, mbus_t *mb)
{
	int	val, reg, n;

	reg = (int)mshort(&mb->buf[2]);
	val = (int)mshort(&mb->buf[4]);
	switch (reg) {
	case 0x100:
		if ((val < 1) || (val > 127)) {
			modbus_faulty(taskarg, mb, 9);	/* illegal data range */
			return -1;
		}
		mregs[MREG_SLAVE_ID] = (uint16_t) val;
		modbus_pack_send(taskarg, mb, 8);
		break;

	case 0x102:
		break;

	case 0x1000:	/* change freqency */
		mregs[MREG_PWM_FREQ] = (uint16_t) val;
		break;

	case 0x1002:	/* change dead time */
		mregs[MREG_PWM_DTIME] = (uint16_t) val;
		break;

	case 0x1004:
	case 0x1006:
		if (reg == 0x1004) {
			mregs[MREG_PWM_DUTY] = (uint16_t) val;
		} else {
			mregs[MREG_PWM_DUTY2] = (uint16_t) val;
		}
		break;
	
	default:
		modbus_faulty(taskarg, mb, 2);	/* illegal data address */
		return -2;
	}
	modbus_pack_send(taskarg, mb, 8);
	return 0;
}

static void modbus_faulty(void *taskarg, mbus_t *mb, int code)
{
	mb->buf[1] |= 0x80;	/* slave id unchanged */
	mb->buf[2] = (char)code;
	modbus_pack_send(taskarg, mb, 5);
}

static void modbus_pack_send(void *taskarg, mbus_t *mb, int n)
{
	uint16_t	crc;

	crc = modbus_crc16(mb->buf, n - 2);
	mb->buf[n-2] = (char)(crc & 0xff);	/* Low byte first in CRC */
	mb->buf[n-1] = (char)((crc >> 8) & 0xff); 
	task_write(taskarg, mb->buf, n);
	mb->stat_tx++;
}

/* calculate the frame length by received data */
static int modbus_frame_length(mbus_t *mb)
{
	if (mb->idx < 2) {
		return 0;	/* need more data */
	}
	switch (mb->buf[1]) {
	/* RTU structure: Slave + func + Register + Quantity + crc */
	case 0x03:	/* Read Holding Registers */
	case 0x04:	/* Read Input Registers */
	case 0x06:	/* Write Single Register */
		return 8; 	/* fixed length */

	/* RTU structure: Slave + func + start_addr(2) + reg_num(2) + byte_count + data + crc */
        case 0x10:	/* Write Multiple Registers */
		if (mb->idx >= 7) {
			return 7 + (unsigned char)mb->buf[6] + 2;
		}
		break;

	case '\n':	/* double tap means to leave the modbus processor */
	case '\r':
		if ((mb->buf[0] == '\n') || (mb->buf[0] == '\r')) {
			return MBUS_STAT_EXIT;
		}
		/* fall over */

        default:
		return MBUS_STAT_RESET;	/* unsupported function code */
	}
	return 0;	/* need more data */
}

static uint16_t mshort(void *buf)
{
	uint8_t	*s = buf;
	return (s[0] << 8) | s[1];
}

static void sshort(void *buf, uint16_t n)
{
	uint8_t	*s = buf;
	s[0] = (n >> 8) & 0xff;
	s[1] = n & 0xff;
}

static void spack(void *buf, uint16_t *val, int n)
{
	uint8_t	*s = buf;

	while (n--) {
		*s++ = (*val >> 8) & 0xff;
		*s++ = *val++ & 0xff;
	}
}


