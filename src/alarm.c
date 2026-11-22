#include "alarm.h"

#include <stdlib.h>
#include <zephyr/sys/util.h>

void alarm_default_cfg(struct alarm_cfg *c)
{
	c->temp_hi_mc = 45000;
	c->temp_clear_mc = 42000;
	c->shock_mg = 1500;
	c->spin_mdps = 200000;
	c->debounce = 3;
}

static int32_t isqrt64(uint64_t v)
{
	uint64_t r = 0, bit = 1ULL << 62;

	while (bit > v) {
		bit >>= 2;
	}
	while (bit) {
		if (v >= r + bit) {
			v -= r + bit;
			r = (r >> 1) + bit;
		} else {
			r >>= 1;
		}
		bit >>= 2;
	}
	return (int32_t)r;
}

static int32_t mag3(const int32_t v[3])
{
	return isqrt64((uint64_t)((int64_t)v[0] * v[0] + (int64_t)v[1] * v[1] + (int64_t)v[2] * v[2]));
}

uint8_t alarm_update(struct alarm_state *st, const struct alarm_cfg *cfg,
		     const struct sample *s, int fault)
{
	/* over-temperature: hysteresis + debounce */
	if (st->active & ALARM_OVERTEMP) {
		if (s->temp_mc < cfg->temp_clear_mc) {
			st->active &= ~ALARM_OVERTEMP;
			st->cnt_temp = 0;
		}
	} else if (s->temp_mc > cfg->temp_hi_mc) {
		if (++st->cnt_temp >= cfg->debounce) {
			st->active |= ALARM_OVERTEMP;
		}
	} else {
		st->cnt_temp = 0;
	}

	/* shock: acceleration magnitude deviates from 1 g (1000 mg) */
	int32_t dev = abs(mag3(s->accel_mg) - 1000);

	if (dev > cfg->shock_mg) {
		if (++st->cnt_shock >= cfg->debounce) {
			st->active |= ALARM_SHOCK;
		}
	} else {
		st->cnt_shock = 0;
		st->active &= ~ALARM_SHOCK;
	}

	/* spin */
	if (mag3(s->gyro_mdps) > cfg->spin_mdps) {
		if (++st->cnt_spin >= cfg->debounce) {
			st->active |= ALARM_SPIN;
		}
	} else {
		st->cnt_spin = 0;
		st->active &= ~ALARM_SPIN;
	}

	if (fault) {
		st->active |= ALARM_SENSOR_FAULT;
	} else {
		st->active &= ~ALARM_SENSOR_FAULT;
	}
	return st->active;
}
