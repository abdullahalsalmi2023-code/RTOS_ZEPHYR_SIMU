#include "frame.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/crc.h>

int frame_encode(const struct sample *s, char *buf, size_t len)
{
	char body[FRAME_MAX_LEN];
	int n = snprintf(body, sizeof(body),
			 "SN,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u",
			 (unsigned)s->seq, (unsigned)s->uptime_ms, (int)s->temp_mc,
			 (int)s->accel_mg[0], (int)s->accel_mg[1], (int)s->accel_mg[2],
			 (int)s->gyro_mdps[0], (int)s->gyro_mdps[1], (int)s->gyro_mdps[2],
			 (int)s->mag_nt[0], (int)s->mag_nt[1], (int)s->mag_nt[2],
			 (unsigned)s->alarms);

	if (n < 0 || (size_t)n >= sizeof(body)) {
		return -ENOMEM;
	}
	uint16_t crc = crc16_itu_t(0, (const uint8_t *)body, (size_t)n);
	int m = snprintf(buf, len, "$%s*%04X\n", body, crc);

	return (m < 0 || (size_t)m >= len) ? -ENOMEM : m;
}

int frame_decode(const char *line, struct sample *out)
{
	const char *star = strrchr(line, '*');

	if (line[0] != '$' || star == NULL) {
		return -EBADMSG;
	}
	size_t body_len = (size_t)(star - (line + 1));
	uint16_t want = (uint16_t)strtoul(star + 1, NULL, 16);

	if (crc16_itu_t(0, (const uint8_t *)(line + 1), body_len) != want) {
		return -EBADMSG;
	}
	unsigned seq, ms, al;
	int t, ax, ay, az, gx, gy, gz, mx, my, mz;

	if (sscanf(line + 1, "SN,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u",
		   &seq, &ms, &t, &ax, &ay, &az, &gx, &gy, &gz, &mx, &my, &mz, &al) != 13) {
		return -EBADMSG;
	}
	out->seq = seq;
	out->uptime_ms = ms;
	out->temp_mc = t;
	out->accel_mg[0] = ax; out->accel_mg[1] = ay; out->accel_mg[2] = az;
	out->gyro_mdps[0] = gx; out->gyro_mdps[1] = gy; out->gyro_mdps[2] = gz;
	out->mag_nt[0] = mx; out->mag_nt[1] = my; out->mag_nt[2] = mz;
	out->alarms = (uint8_t)al;
	return 0;
}
