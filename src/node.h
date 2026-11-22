#ifndef NODE_H_
#define NODE_H_

#include <zephyr/kernel.h>
#include "alarm.h"
#include "frame.h"

struct node_state {
	struct k_mutex lock;
	uint32_t period_ms;
	struct alarm_cfg cfg;
	struct alarm_state alarm;
	struct sample last;
	uint32_t frames, read_errors;
};

extern struct node_state node;

#endif
