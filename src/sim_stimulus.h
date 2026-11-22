/* Virtual world: drives the emulated sensor chips so the firmware sees plausible physics.
 * Compiled for every build, but only has an effect where emulators exist (native_sim). */
#ifndef SIM_STIMULUS_H_
#define SIM_STIMULUS_H_

#include <stdint.h>

int sim_init(void);
void sim_step(uint32_t uptime_ms);
void sim_set_heat_mc(int32_t extra_mc);   /* adds to ambient temperature */
void sim_trigger_shock(uint32_t duration_ms);
void sim_trigger_spin(uint32_t duration_ms);
void sim_reset(void);

#endif
