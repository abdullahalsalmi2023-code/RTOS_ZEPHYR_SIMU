/*
 * Multi-sensor node: periodically reads a temperature sensor, an IMU and a
 * magnetometer over I2C, evaluates alarms, and streams CRC-protected frames on
 * the UART. A UART shell allows runtime control. Runs on native_sim today;
 * the same sources build for real boards by adding a board overlay.
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "alarm.h"
#include "frame.h"
#include "node.h"
#include "sensors.h"
#include "sim_stimulus.h"

struct node_state node;

int main(void)
{
	k_mutex_init(&node.lock);
	node.period_ms = 500;
	alarm_default_cfg(&node.cfg);

	if (sensors_init() != 0 || sim_init() != 0) {
		printk("# FATAL: sensor bring-up failed\n");
		return -1;
	}
	printk("# sensor-node up, period=%u ms\n", node.period_ms);

	uint32_t seq = 0;
	int64_t next = k_uptime_get();

	for (;;) {
		struct sample s = {0};
		char line[FRAME_MAX_LEN];

		s.seq = seq++;
		s.uptime_ms = (uint32_t)k_uptime_get();
		sim_step(s.uptime_ms);

		int rc = sensors_read(&s);

		k_mutex_lock(&node.lock, K_FOREVER);
		s.alarms = alarm_update(&node.alarm, &node.cfg, &s, rc != 0);
		node.last = s;
		node.frames++;
		if (rc != 0) {
			node.read_errors++;
		}
		uint32_t period = node.period_ms;

		k_mutex_unlock(&node.lock);

		if (frame_encode(&s, line, sizeof(line)) > 0) {
			printk("%s", line);
		}

		next += period;
		int64_t wait = next - k_uptime_get();

		if (wait < 0) { /* overrun: resync instead of bursting */
			next = k_uptime_get();
			wait = 0;
		}
		k_sleep(K_MSEC(wait));
	}
	return 0;
}
