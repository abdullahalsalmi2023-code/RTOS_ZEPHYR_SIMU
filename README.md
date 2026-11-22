# Zephyr Multi-Sensor Node (simulated)

Embedded firmware in C on **Zephyr RTOS 4.2** that reads a temperature sensor, an IMU and a magnetometer over **I2C**, evaluates alarms, and streams CRC-protected telemetry over **UART**, with a UART shell for runtime control. It runs entirely on a PC using Zephyr's `native_sim` target, with the I2C chips emulated, so no hardware is needed.

> **Simulation, stated plainly:** the sensor chips and I2C bus are emulated. The application code, device tree, and the three sensor drivers (F75303, BMI160, AKM09918C) are the real upstream Zephyr ones. This has not been run on physical hardware.

## What it demonstrates
- Zephyr device tree overlays, Kconfig, `west` application repo pinned to a release
- I2C sensor drivers through the Zephyr sensor API, over Zephyr's I2C emulator
- Periodic real-time loop with drift-free scheduling, mutex-protected shared state
- Alarm logic with debounce and hysteresis; fixed-point integer telemetry with CRC-16
- UART shell commands and fault injection (heat, shock, spin)
- Unit tests (ztest) and an end-to-end UART scenario test, both run in CI

## Run it
Requires a Zephyr workspace (see below), `gcc`, `cmake`, `ninja`, `dtc`, `gperf`.
```bash
export ZEPHYR_TOOLCHAIN_VARIANT=host
west build -b native_sim/native/64 sensor-node -d build/app
./build/app/zephyr/zephyr.exe
```
Type commands in the same terminal, e.g. `node period 100`, `sim heat 20`, `sim shock`, `node status`.

Example output:
```
# sensor-node up, period=500 ms
$SN,0,50,28000,1,4,1000,0,0,114,29800,0,-34900,0*6446
$SN,1,560,28000,-3,-4,1002,0,0,114,29800,1000,-34900,0*DBC0
```
Frame format and shell commands: [docs/PROTOCOL.md](docs/PROTOCOL.md). Design notes: [docs/DESIGN.md](docs/DESIGN.md).

## Workspace setup and tests
```bash
mkdir ws && cd ws
git clone <this repo> sensor-node
pip install west && west init -l sensor-node && west update --narrow -o=--depth=1
pip install -r zephyr/scripts/requirements-base.txt
sensor-node/scripts/build_and_test.sh
```
That builds and runs 11 ztest unit tests and a 12-check scenario test (`tools/uart_monitor.py`) that drives the shell, injects faults, validates every CRC, checks sequence continuity and verifies each alarm trips and clears. The same steps run in GitHub Actions (`.github/workflows/ci.yml`).

## Layout
`src/` firmware | `boards/` device tree overlay and config for native_sim | `tests/unit/` ztest | `tools/` UART monitor/verifier | `docs/` protocol and design

## Limitations
- Simulated only (native_sim, not QEMU or Renode, and not real silicon).
- Sensor fault alarm path is unit-tested but there is no bus-fault injection yet.
- The virtual world is a simple deterministic model, not recorded real data.

License: MIT
