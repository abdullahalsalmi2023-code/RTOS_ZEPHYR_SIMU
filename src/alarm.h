/* Alarm evaluation with hysteresis and debounce. Pure logic: unit-testable off-target. */
#ifndef ALARM_H_
#define ALARM_H_

#include <stdint.h>
#include "frame.h"

#define ALARM_OVERTEMP  BIT(0)
#define ALARM_SHOCK     BIT(1)
#define ALARM_SPIN      BIT(2)
#define ALARM_SENSOR_FAULT BIT(7)

struct alarm_cfg {
	int32_t temp_hi_mc;     /* trip above */
	int32_t temp_clear_mc;  /* clear below (hysteresis) */
	int32_t shock_mg;       /* |a| deviation from 1 g that trips shock */
	int32_t spin_mdps;      /* |w| that trips spin */
	uint8_t debounce;       /* consecutive samples required to trip */
};

struct alarm_state {
	uint8_t active;
	uint8_t cnt_temp, cnt_shock, cnt_spin;
};

void alarm_default_cfg(struct alarm_cfg *c);
/* Updates state from a sample; returns active mask. `fault` forces SENSOR_FAULT. */
uint8_t alarm_update(struct alarm_state *st, const struct alarm_cfg *cfg,
		     const struct sample *s, int fault);

#endif
