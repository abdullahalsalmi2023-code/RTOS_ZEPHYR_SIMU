#ifndef SENSORS_H_
#define SENSORS_H_

#include "frame.h"

int sensors_init(void);
/* Reads all sensors over I2C. Returns 0 if every device answered, else negative errno
 * (sample fields of failed devices are left zero). */
int sensors_read(struct sample *s);

#endif
