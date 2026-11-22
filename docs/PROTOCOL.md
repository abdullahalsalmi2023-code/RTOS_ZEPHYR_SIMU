# UART telemetry protocol

One ASCII line per sample, newline terminated. All values are integers (no floats on the wire).

```
$SN,<seq>,<ms>,<temp_mC>,<ax>,<ay>,<az>,<gx>,<gy>,<gz>,<mx>,<my>,<mz>,<alarms>*<CRC16>
```

| Field | Unit |
|---|---|
| seq | frame counter, increments by 1 (gaps = lost frames) |
| ms | node uptime in milliseconds |
| temp_mC | milli-degrees Celsius |
| ax, ay, az | milli-g |
| gx, gy, gz | milli-degrees per second |
| mx, my, mz | nano-tesla |
| alarms | bit mask: 0x01 over-temp, 0x02 shock, 0x04 spin, 0x80 sensor fault |
| CRC16 | 4 hex digits, CRC-16/ITU-T (poly 0x1021, init 0) over the bytes between `$` and `*` |

Lines starting with `#` are human-readable diagnostics and must be ignored by parsers.

## Shell commands (same UART)
| Command | Effect |
|---|---|
| `node status` | last sample, counters, alarm mask |
| `node period <ms>` | sampling period, 20..10000 |
| `node thresh_temp <C>` | over-temp trip level (clears 3 C lower) |
| `sim heat <dC>` | add heat to the virtual environment (simulation only) |
| `sim shock [ms]` | inject an impact |
| `sim spin [ms]` | inject rotation |
| `sim reset` | clear injected faults |
