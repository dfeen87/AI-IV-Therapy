# Alert Log Format

## Overview
Alert events are emitted as single-line NDJSON entries in `*_system.log`, prefixed with `ALERT `. This makes them easy to filter with log tailers while preserving the existing log stream.

## Line Format
```
ALERT {"timestamp":<epoch_ms>,"severity":"<DEBUG|INFO|WARN|ERROR|CRITICAL>","source":"<component>","code":"<STABLE_CODE>","message":"<human text>","context":{...}}
```

- `timestamp` is epoch milliseconds.
- `severity` uses a fixed, uppercase enum.
- `source` is the emitting component/module name.
- `code` is a short, stable identifier.
- `message` is a human-readable summary.
- `context` is optional and, when present, is a JSON object with lightweight key/value data.

## Examples
```
ALERT {"timestamp":1731283025123,"severity":"WARN","source":"Telemetry","code":"SENSOR_QUALITY_LOW","message":"Telemetry signal quality below threshold","context":{"signal_quality":0.55,"threshold":0.6}}
ALERT {"timestamp":1731283025350,"severity":"WARN","source":"SafetyMonitor","code":"LOW_CARDIAC_RESERVE","message":"Cardiac reserve below minimum threshold","context":{"cardiac_reserve":0.18}}
ALERT {"timestamp":1731283025601,"severity":"CRITICAL","source":"SafetyMonitor","code":"EMERGENCY_MIN_RATE","message":"Emergency minimum infusion rate enforced","context":{"hydration_pct":42.1}}
```

## Filtering
```
tail -F ai_iv_<session>_system.log | rg '^ALERT '
```
