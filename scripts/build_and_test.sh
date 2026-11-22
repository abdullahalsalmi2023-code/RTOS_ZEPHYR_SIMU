#!/usr/bin/env bash
# Build firmware + unit tests for native_sim, run both. Expects an initialised west workspace
# (see README) and ZEPHYR_TOOLCHAIN_VARIANT=host (uses your system gcc).
set -euo pipefail
export ZEPHYR_TOOLCHAIN_VARIANT=${ZEPHYR_TOOLCHAIN_VARIANT:-host}
APP="$(cd "$(dirname "$0")/.." && pwd)"
BOARD=native_sim/native/64

west build -b "$BOARD" "$APP/tests/unit" -d build/unit -p auto
./build/unit/zephyr/zephyr.exe | tail -5

west build -b "$BOARD" "$APP" -d build/app -p auto
python3 "$APP/tools/uart_monitor.py" build/app/zephyr/zephyr.exe
