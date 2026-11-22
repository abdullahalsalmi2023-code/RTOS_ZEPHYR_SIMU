# Design notes

## Architecture
```
 virtual world (sim_stimulus.c)      <- writes physics into emulated chips
        |
 emulated I2C chips (Zephyr emul: F75303 temp @0x4d, BMI160 IMU @0x68, AKM09918C mag @0x0c)
        |   I2C transactions through Zephyr's i2c-emul controller
 real Zephyr sensor drivers (F75303, BMI160, AKM09918C)    <- unmodified upstream drivers
        |   sensor_sample_fetch / sensor_channel_get
 sensors.c  -> unit conversion to integers (mC, mg, mdps, nT)
        |
 main loop (drift-free periodic scheduling) -> alarm.c (debounce + hysteresis) -> frame.c (CRC16)
        |
 UART0 telemetry + shell (cmd_shell.c): runtime period / threshold / fault injection
```

## Decisions worth explaining in an interview
- **Real drivers, emulated silicon.** Only the chip is simulated; the driver code, device tree and sensor API calls are the same as on hardware. Moving to a real board means a new overlay, not new application code.
- **Pure-logic modules.** `frame.c` and `alarm.c` have no hardware dependency, so they are unit-tested with ztest.
- **Integer fixed-point on the wire.** No float formatting, deterministic parsing, small frames.
- **Hysteresis and debounce.** The over-temperature alarm needs 3 consecutive samples to trip and clears 3 C lower, so it does not chatter.
- **Drift-free scheduling.** The loop computes the next deadline from the previous deadline, not from "now", and resyncs after an overrun instead of bursting.
- **CRC on every frame.** A host can detect corruption; the sequence number detects loss.
- **Accel range.** The IMU powers on at +-2 g, which clips impacts; firmware sets +-8 g at init. The scenario test caught this bug.

## Porting to a real board (not done, untested)
1. Add `boards/<board>.overlay` placing the same three nodes on a real I2C bus.
2. Add the needed HAL module to `west.yml` and the I2C controller config to `prj.conf`.
3. `sim_stimulus.c` compiles to no-ops when `CONFIG_EMUL` is off.
