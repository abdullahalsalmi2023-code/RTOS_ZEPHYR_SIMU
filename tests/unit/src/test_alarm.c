#include <zephyr/ztest.h>
#include <string.h>
#include "alarm.h"

static struct alarm_cfg cfg;
static struct alarm_state st;

static void before(void *f)
{
	alarm_default_cfg(&cfg);
	memset(&st, 0, sizeof(st));
}

ZTEST_SUITE(alarm, NULL, NULL, before, NULL, NULL);

static struct sample rest(void)
{
	return (struct sample){.temp_mc = 25000, .accel_mg = {0, 0, 1000}};
}

ZTEST(alarm, test_quiet_at_rest)
{
	struct sample s = rest();

	for (int i = 0; i < 20; i++) {
		zassert_equal(alarm_update(&st, &cfg, &s, 0), 0);
	}
}

ZTEST(alarm, test_overtemp_debounce_and_hysteresis)
{
	struct sample s = rest();

	s.temp_mc = 46000;
	zassert_equal(alarm_update(&st, &cfg, &s, 0), 0, "1st sample must not trip");
	zassert_equal(alarm_update(&st, &cfg, &s, 0), 0, "2nd sample must not trip");
	zassert_true(alarm_update(&st, &cfg, &s, 0) & ALARM_OVERTEMP, "3rd trips");

	s.temp_mc = 43000; /* between clear (42) and trip (45): stays on */
	zassert_true(alarm_update(&st, &cfg, &s, 0) & ALARM_OVERTEMP);
	s.temp_mc = 41000;
	zassert_false(alarm_update(&st, &cfg, &s, 0) & ALARM_OVERTEMP);
}

ZTEST(alarm, test_single_spike_does_not_trip)
{
	struct sample s = rest();

	s.temp_mc = 60000;
	alarm_update(&st, &cfg, &s, 0);
	s.temp_mc = 25000;
	for (int i = 0; i < 5; i++) {
		zassert_equal(alarm_update(&st, &cfg, &s, 0), 0);
	}
}

ZTEST(alarm, test_shock)
{
	struct sample s = rest();

	s.accel_mg[0] = 3000; /* ~3.2 g total */
	uint8_t a = 0;

	for (int i = 0; i < 3; i++) {
		a = alarm_update(&st, &cfg, &s, 0);
	}
	zassert_true(a & ALARM_SHOCK);
	s = rest();
	zassert_false(alarm_update(&st, &cfg, &s, 0) & ALARM_SHOCK);
}

ZTEST(alarm, test_spin)
{
	struct sample s = rest();

	s.gyro_mdps[2] = 300000;
	uint8_t a = 0;

	for (int i = 0; i < 3; i++) {
		a = alarm_update(&st, &cfg, &s, 0);
	}
	zassert_true(a & ALARM_SPIN);
}

ZTEST(alarm, test_sensor_fault_flag)
{
	struct sample s = rest();

	zassert_true(alarm_update(&st, &cfg, &s, 1) & ALARM_SENSOR_FAULT);
	zassert_false(alarm_update(&st, &cfg, &s, 0) & ALARM_SENSOR_FAULT);
}
