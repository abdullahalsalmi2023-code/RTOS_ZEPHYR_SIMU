#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include "node.h"
#include "sim_stimulus.h"

static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
	k_mutex_lock(&node.lock, K_FOREVER);
	struct sample s = node.last;
	uint32_t p = node.period_ms, f = node.frames, e = node.read_errors;
	uint8_t a = node.alarm.active;

	k_mutex_unlock(&node.lock);
	shell_print(sh, "period=%ums frames=%u i2c_errors=%u alarms=0x%02x", p, f, e, a);
	shell_print(sh, "temp=%d.%03dC accel_mg=(%d,%d,%d) gyro_mdps=(%d,%d,%d)",
		    s.temp_mc / 1000, abs(s.temp_mc % 1000), s.accel_mg[0], s.accel_mg[1],
		    s.accel_mg[2], s.gyro_mdps[0], s.gyro_mdps[1], s.gyro_mdps[2]);
	return 0;
}

static int cmd_period(const struct shell *sh, size_t argc, char **argv)
{
	long ms = strtol(argv[1], NULL, 10);

	if (ms < 20 || ms > 10000) {
		shell_error(sh, "period must be 20..10000 ms");
		return -EINVAL;
	}
	k_mutex_lock(&node.lock, K_FOREVER);
	node.period_ms = (uint32_t)ms;
	k_mutex_unlock(&node.lock);
	shell_print(sh, "period=%ld ms", ms);
	return 0;
}

static int cmd_thresh_temp(const struct shell *sh, size_t argc, char **argv)
{
	long c = strtol(argv[1], NULL, 10);

	if (c < -40 || c > 125) {
		shell_error(sh, "threshold must be -40..125 C");
		return -EINVAL;
	}
	k_mutex_lock(&node.lock, K_FOREVER);
	node.cfg.temp_hi_mc = c * 1000;
	node.cfg.temp_clear_mc = (c - 3) * 1000;
	k_mutex_unlock(&node.lock);
	shell_print(sh, "over-temp trips above %ld C, clears below %ld C", c, c - 3);
	return 0;
}

static int cmd_heat(const struct shell *sh, size_t argc, char **argv)
{
	sim_set_heat_mc((int32_t)(strtol(argv[1], NULL, 10) * 1000));
	return 0;
}

static int cmd_shock(const struct shell *sh, size_t argc, char **argv)
{
	sim_trigger_shock(argc > 1 ? (uint32_t)strtoul(argv[1], NULL, 10) : 500);
	return 0;
}

static int cmd_spin(const struct shell *sh, size_t argc, char **argv)
{
	sim_trigger_spin(argc > 1 ? (uint32_t)strtoul(argv[1], NULL, 10) : 1000);
	return 0;
}

static int cmd_reset(const struct shell *sh, size_t argc, char **argv)
{
	sim_reset();
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(node_cmds,
	SHELL_CMD_ARG(status, NULL, "Show last sample and counters", cmd_status, 1, 0),
	SHELL_CMD_ARG(period, NULL, "period <ms>  sampling period (20..10000)", cmd_period, 2, 0),
	SHELL_CMD_ARG(thresh_temp, NULL, "thresh_temp <C>  over-temp alarm threshold", cmd_thresh_temp, 2, 0),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(node, &node_cmds, "Sensor node control", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(sim_cmds,
	SHELL_CMD_ARG(heat, NULL, "heat <dC>  add dC degrees to ambient", cmd_heat, 2, 0),
	SHELL_CMD_ARG(shock, NULL, "shock [ms]  inject an impact", cmd_shock, 1, 1),
	SHELL_CMD_ARG(spin, NULL, "spin [ms]  inject rotation", cmd_spin, 1, 1),
	SHELL_CMD_ARG(reset, NULL, "clear injected faults", cmd_reset, 1, 0),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(sim, &sim_cmds, "Virtual-world fault injection", NULL);
