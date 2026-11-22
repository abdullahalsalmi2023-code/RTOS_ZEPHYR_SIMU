/* Telemetry frame encoder/decoder. Pure C, no Zephyr kernel dependencies except crc16. */
#ifndef FRAME_H_
#define FRAME_H_

#include <stddef.h>
#include <stdint.h>

/* All values are fixed-point integers (no floats on the wire). */
struct sample {
	uint32_t seq;
	uint32_t uptime_ms;
	int32_t temp_mc;      /* milli-degrees C */
	int32_t accel_mg[3];  /* milli-g */
	int32_t gyro_mdps[3]; /* milli-degrees/s */
	int32_t mag_nt[3];    /* nano-tesla */
	uint8_t alarms;       /* bit mask, see alarm.h */
};

/* Frame format:  $SN,<seq>,<ms>,<tmC>,<ax>,<ay>,<az>,<gx>,<gy>,<gz>,<mx>,<my>,<mz>,<alarms>*<CRC16 hex4>\n
 * CRC-16/ITU-T (CCITT, poly 0x1021, init 0) over the bytes between '$' and '*'. */
#define FRAME_MAX_LEN 160

/* Returns length written (excluding NUL) or negative errno. */
int frame_encode(const struct sample *s, char *buf, size_t len);

/* Returns 0 on success, -EBADMSG on CRC/format error. */
int frame_decode(const char *line, struct sample *out);

#endif
