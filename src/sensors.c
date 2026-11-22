#include "sensors.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

static const struct device *const temp_dev = DEVICE_DT_GET(DT_NODELABEL(temp0));
static const struct device *const imu_dev = DEVICE_DT_GET(DT_NODELABEL(imu0));
static const struct device *const mag_dev = DEVICE_DT_GET(DT_NODELABEL(mag0));

#define M_S2_PER_G_MILLI 9807 /* 1 g = 9.80665 m/s^2 -> value_in_m_s2 * 1000 / 9.807 = mg */

static int32_t sv_milli(const struct sensor_value *v)
{
	return (int32_t)(v->val1 * 1000 + v->val2 / 1000);
}

int sensors_init(void)
{
	const struct device *devs[] = {temp_dev, imu_dev, mag_dev};
	const char *names[] = {"F75303 temp", "BMI160 imu", "AKM09918C mag"};

	for (int i = 0; i < 3; i++) {
		if (!device_is_ready(devs[i])) {
			printk("# sensor not ready: %s\n", names[i]);
			return -ENODEV;
		}
	}
	/* +-8 g so impacts are not clipped by the power-on default of +-2 g */
	struct sensor_value fs;

	sensor_g_to_ms2(8, &fs);
	int rc = sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &fs);

	if (rc != 0) {
		printk("# warn: could not set accel range (%d)\n", rc);
	}
	return 0;
}

int sensors_read(struct sample *s)
{
	struct sensor_value v[3];
	int err = 0, rc;

	rc = sensor_sample_fetch(temp_dev);
	if (rc == 0) {
		rc = sensor_channel_get(temp_dev, SENSOR_CHAN_AMBIENT_TEMP, &v[0]);
	}
	if (rc == 0) {
		s->temp_mc = sv_milli(&v[0]);
	} else {
		err = rc;
	}

	rc = sensor_sample_fetch(imu_dev);
	if (rc == 0) {
		rc = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, v);
		if (rc == 0) {
			for (int i = 0; i < 3; i++) {
				/* m/s^2 (milli) -> milli-g */
				s->accel_mg[i] = (int32_t)((int64_t)sv_milli(&v[i]) * 1000 / M_S2_PER_G_MILLI);
			}
		}
		if (rc == 0) {
			rc = sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, v);
		}
		if (rc == 0) {
			for (int i = 0; i < 3; i++) {
				/* rad/s (milli) -> milli-deg/s */
				s->gyro_mdps[i] = (int32_t)((int64_t)sv_milli(&v[i]) * 57296 / 1000);
			}
		}
	}
	if (rc != 0) {
		err = rc;
	}

	rc = sensor_sample_fetch(mag_dev);
	if (rc == 0) {
		rc = sensor_channel_get(mag_dev, SENSOR_CHAN_MAGN_XYZ, v);
	}
	if (rc == 0) {
		for (int i = 0; i < 3; i++) {
			/* gauss -> nano-tesla: 1 G = 100000 nT */
			s->mag_nt[i] = (int32_t)((int64_t)sv_milli(&v[i]) * 100);
		}
	} else {
		err = rc;
	}
	return err;
}
