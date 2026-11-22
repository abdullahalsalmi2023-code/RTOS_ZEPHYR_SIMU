#include "sim_stimulus.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#ifdef CONFIG_EMUL

#include <math.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#define SHIFT 8

static const struct emul *temp_emul = EMUL_DT_GET(DT_NODELABEL(temp0));
static const struct emul *imu_emul = EMUL_DT_GET(DT_NODELABEL(imu0));
static const struct emul *mag_emul = EMUL_DT_GET(DT_NODELABEL(mag0));

static int32_t heat_mc;
static uint32_t shock_until, spin_until;

static q31_t to_q31(double v)
{
	/* value = q31 / 2^31 * 2^SHIFT */
	return (q31_t)((v / (double)(1 << SHIFT)) * 2147483648.0);
}

static void set(const struct emul *e, enum sensor_channel c, double v)
{
	q31_t q = to_q31(v);

	emul_sensor_backend_set_channel(e, (struct sensor_chan_spec){c, 0}, &q, SHIFT);
}

int sim_init(void)
{
	heat_mc = 0;
	shock_until = spin_until = 0;
	return (temp_emul && imu_emul && mag_emul) ? 0 : -ENODEV;
}

void sim_reset(void)
{
	heat_mc = 0;
	shock_until = spin_until = 0;
}

void sim_set_heat_mc(int32_t extra_mc) { heat_mc = extra_mc; }
void sim_trigger_shock(uint32_t d) { shock_until = k_uptime_get_32() + d; }
void sim_trigger_spin(uint32_t d) { spin_until = k_uptime_get_32() + d; }

void sim_step(uint32_t t_ms)
{
	double t = t_ms / 1000.0;

	/* ambient: 28 C with slow +-1.5 C drift, plus injected heat */
	double temp_c = 28.0 + 1.5 * sin(t / 20.0) + heat_mc / 1000.0;

	set(temp_emul, SENSOR_CHAN_AMBIENT_TEMP, temp_c < 0 ? 0 : temp_c);

	/* IMU at rest on a table: ~1 g on Z, tiny deterministic vibration */
	double ax = 0.05 * sin(t * 7.0), ay = 0.05 * cos(t * 5.0), az = 9.80665 + 0.03 * sin(t * 3.0);

	if (t_ms < shock_until) {
		ax += 28.0; /* ~2.9 g impact */
	}
	set(imu_emul, SENSOR_CHAN_ACCEL_X, ax);
	set(imu_emul, SENSOR_CHAN_ACCEL_Y, ay);
	set(imu_emul, SENSOR_CHAN_ACCEL_Z, az);

	double w = (t_ms < spin_until) ? 5.0 : 0.002; /* rad/s: 5 rad/s ~ 286 dps */

	set(imu_emul, SENSOR_CHAN_GYRO_X, 0.0);
	set(imu_emul, SENSOR_CHAN_GYRO_Y, 0.0);
	set(imu_emul, SENSOR_CHAN_GYRO_Z, w);

	/* Earth-like field, ~0.45 gauss, rotating slowly with heading */
	set(mag_emul, SENSOR_CHAN_MAGN_X, 0.30 * cos(t / 15.0));
	set(mag_emul, SENSOR_CHAN_MAGN_Y, 0.30 * sin(t / 15.0));
	set(mag_emul, SENSOR_CHAN_MAGN_Z, -0.35);
}

#else /* real hardware: nothing to simulate */

int sim_init(void) { return 0; }
void sim_step(uint32_t t_ms) { ARG_UNUSED(t_ms); }
void sim_set_heat_mc(int32_t e) { ARG_UNUSED(e); }
void sim_trigger_shock(uint32_t d) { ARG_UNUSED(d); }
void sim_trigger_spin(uint32_t d) { ARG_UNUSED(d); }
void sim_reset(void) {}

#endif
