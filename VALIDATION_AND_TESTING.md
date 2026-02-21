# Validation and Testing

**AI-Optimized Intravenous Therapy Control System — v4.0.0**  
**Status:** Pre-Clinical Research Reference  
**Language:** C++17  

> **Important:** This document applies to a pre-clinical research system. It does not constitute
> clinical validation, regulatory evidence, or a claim of medical device approval.

---

## Table of Contents

1. [Purpose](#1-purpose)
2. [System Under Test](#2-system-under-test)
3. [Build Verification](#3-build-verification)
4. [Test Suite](#4-test-suite)
   - 4.1 [SafetyMonitor Tests](#41-safetymonitor-tests)
   - 4.2 [StateEstimator Tests](#42-stateestimator-tests)
   - 4.3 [Neural Estimator Tests](#43-neural-estimator-tests)
5. [Test Results Summary](#5-test-results-summary)
6. [Safety Invariant Analysis](#6-safety-invariant-analysis)
7. [Configuration Constants and Bounds](#7-configuration-constants-and-bounds)
8. [Failure Mode Analysis](#8-failure-mode-analysis)
9. [Known Gaps and Limitations](#9-known-gaps-and-limitations)
10. [Path to Clinical Validation](#10-path-to-clinical-validation)

---

## 1. Purpose

This document provides a structured analysis, test documentation, and validation summary for the
AI-Optimized Intravenous Therapy Control System. It covers:

- Build verification across all supported targets
- Documented test cases with expected and actual results
- Safety invariant verification
- Configuration constant review
- Honest assessment of coverage gaps

This document is designed to:

- Support research review and collaboration
- Enable safety-critical code inspection
- Provide a transparent baseline for future regulatory work

---

## 2. System Under Test

### Module Inventory

| Module | Source File(s) | Role |
|---|---|---|
| `StateEstimator` | `src/StateEstimator.cpp` / `.hpp` | Signal fusion, coherence scoring, energy proxy, cardiac reserve, risk scoring |
| `AdaptiveController` | `src/AdaptiveController.cpp` / `.hpp` | Demand modeling, coherence modulation, cardiac limiting, predictive boost |
| `SafetyMonitor` | `src/SafetyMonitor.cpp` / `.hpp` | Volume limits, cardiac load, rate-of-change, emergency overrides |
| `SystemLogger` | `src/SystemLogger.cpp` / `.hpp` | Structured NDJSON alert events, telemetry CSV, control CSV |
| `NeuralStateEstimator` | `src/NeuralStateEstimator.hpp` | 241-parameter feedforward network (optional, `frugally-deep`) |
| `SimulationEngine` | `src/simulation_engine.cpp` / `.hpp` | Parameterized physiological scenario driver |
| `RestApiServer` | `src/rest_api_server.cpp` / `.hpp` | Read-only HTTP API (optional, `-DENABLE_REST_API`) |

### Data Contracts

All inter-module data is exchanged through stable types defined in `src/iv_system_types.hpp`:

| Type | Purpose |
|---|---|
| `Telemetry` | Raw sensor snapshot (hydration, HR, SpO₂, temp, lactate, fatigue, signal quality) |
| `PatientProfile` | Static patient parameters (weight, age, conditions, safety limits) |
| `PatientState` | Derived physiological state (coherence, energy proxy, cardiac reserve, risk score) |
| `ControlOutput` | Infusion command with confidence and rationale string |
| `AlertEvent` | Structured NDJSON alert with severity, code, source, message |

### Centralized Configuration

All tunable parameters are defined in `src/config_defaults.hpp`. No magic numbers appear in
control or safety logic.

---

## 3. Build Verification

### Targets

| Target | Command | Description |
|---|---|---|
| Core binary | `make all` | Standard build, strict warnings |
| Test binaries | `make test` | Compiles and runs `test_safety_monitor` + `test_state_estimator` |
| Neural-enabled binary | `make neural` | Requires `libfdeep-dev`, `libeigen3-dev` |
| Neural tests | `make test_all` | Includes `test_neural_estimator` |
| Clean | `make clean` | Removes all build artifacts |

### Compiler Flags

```
g++ -std=c++17 -O2 -Wall -Wextra -pthread
```

All three flags (`-Wall`, `-Wextra`, and the C++17 standard) are enforced. The build produces
**zero warnings** on the standard target.

### Build Results (v4.0.0)

| Target | Result |
|---|---|
| `make all` | ✅ Pass — zero warnings |
| `make test` | ✅ Pass — all unit tests pass |
| `make neural` | Requires optional `libfdeep-dev` / `libeigen3-dev` apt packages |

---

## 4. Test Suite

Tests are located in the `tests/` directory and compiled via `make test` or `make test_all`.
Each test binary exits with code `0` on success and calls `exit(1)` on any assertion failure,
making failures unambiguous in CI.

### 4.1 SafetyMonitor Tests

**File:** `tests/test_safety_monitor.cpp`  
**Binary:** `./test_safety_monitor`

#### Test: `test_volume_limit`

Verifies that the `SafetyMonitor` reduces `max_allowed_rate` and emits a `VOLUME_LIMIT_APPROACH`
warning when cumulative infused volume exceeds 90% of the patient's 24-hour limit.

| Parameter | Value |
|---|---|
| Patient weight | 70 kg |
| Cardiac condition | false |
| Renal impairment | false |
| Max safe infusion rate | 1.5 ml/min |
| 24-hour volume limit | 70 × 35 = 2450 ml |
| 90% warning threshold | 2205 ml |
| Pre-loaded volume | 2300 ml (via `update_volume(100.0, 23.0)`) |

**Expected behaviour:**
- `max_allowed_rate` reduced to ≤ 0.3 ml/min
- `warnings` string contains `VOLUME_LIMIT_APPROACH`

**Result:** ✅ Pass

---

#### Test: `test_cardiac_reserve`

Verifies that the `SafetyMonitor` reduces `max_allowed_rate` and emits a `LOW_CARDIAC_RESERVE`
warning when the patient's cardiac reserve falls below the minimum threshold (`0.2`).

| Parameter | Value |
|---|---|
| Patient weight | 70 kg |
| Cardiac condition | false |
| Max safe infusion rate | 1.5 ml/min |
| Injected cardiac reserve | 0.1 (critically low) |

**Expected behaviour:**
- `max_allowed_rate` reduced to ≤ 0.5 ml/min
- `warnings` string contains `LOW_CARDIAC_RESERVE`

**Result:** ✅ Pass

---

### 4.2 StateEstimator Tests

**File:** `tests/test_state_estimator.cpp`  
**Binary:** `./test_state_estimator`

#### Test: `test_estimate_basic`

Verifies that `StateEstimator::estimate` correctly passes through hydration and heart rate from
a telemetry snapshot, and that the uncertainty estimate is reasonable for a healthy, high
signal-quality input.

| Input | Value |
|---|---|
| `hydration_pct` | 80.0 % |
| `heart_rate_bpm` | 75.0 bpm |
| `temp_celsius` | 37.0 °C |
| `spo2_pct` | 98.0 % |
| `signal_quality` | 1.0 (perfect) |
| Patient weight | 70 kg |
| Baseline HR | 70 bpm |

**Expected behaviour:**
- `state.hydration_pct` within 0.1 of 80.0
- `state.heart_rate_bpm` equals 75.0
- `state.uncertainty` < 0.5 (low uncertainty under perfect signal quality)

**Result:** ✅ Pass

---

### 4.3 Neural Estimator Tests

**File:** `tests/test_neural_estimator.cpp`  
**Binary:** `./test_neural_estimator`  
**Requires:** `-DENABLE_NEURAL_ESTIMATOR`, `libfdeep-dev`, `libeigen3-dev`,  
`models/sensor_fusion_fdeep.json`

The neural estimator is a 241-parameter feedforward network
(Dense-16-ReLU → Dense-8-ReLU → Dense-1-Sigmoid) trained via knowledge distillation from the
hand-crafted rule-based formula.

#### Test: `test_load_and_healthy_patient`

Loads the `frugally-deep` JSON model and runs inference for a healthy patient.

| Input (normalised) | Value |
|---|---|
| `hydration_norm` | 0.80 (80%) |
| `hr_norm` | 0.375 (75 bpm / 200) |
| `spo2_norm` | 0.98 |
| `lactate_norm` | 0.10 (2 mmol/L / 20) |
| `fatigue` | 0.30 |

**Expected:** `E_T` > 0.6 and ≤ 1.0

---

#### Test: `test_stressed_patient`

Runs inference for a severely stressed patient.

| Input (normalised) | Value |
|---|---|
| `hydration_norm` | 0.40 (40%) |
| `hr_norm` | 0.65 (130 bpm / 200) |
| `spo2_norm` | 0.84 |
| `lactate_norm` | 0.40 (8 mmol/L / 20) |
| `fatigue` | 0.90 |

**Expected:** `E_T` < 0.5 and ≥ 0.0

---

#### Test: `test_rule_formula_agreement`

Verifies that the neural model agrees with the reference rule-based formula to within a
mean absolute error (MAE) of 0.08 across three representative spot-check samples. This
threshold matches the training target in `tools/train_sensor_fusion_model.py`.

| Sample | hydration | HR | SpO₂ | lactate | fatigue | Reference E_T |
|---|---|---|---|---|---|---|
| 0 | 70% | 70 bpm | 98% | 2 mmol/L | 0.20 | ~0.85 |
| 1 | 50% | 100 bpm | 92% | 5 mmol/L | 0.60 | ~0.55 |
| 2 | 90% | 80 bpm | 99% | 1 mmol/L | 0.10 | ~0.88 |

**Expected:** MAE across all three samples < 0.08

> These tests require the optional `frugally-deep` runtime and are run via `make test_all`.
> The standard CI gate (`make test`) covers only the deterministic (non-neural) tests.

---

## 5. Test Results Summary

| Test | Module | Result |
|---|---|---|
| `test_volume_limit` | `SafetyMonitor` | ✅ Pass |
| `test_cardiac_reserve` | `SafetyMonitor` | ✅ Pass |
| `test_estimate_basic` | `StateEstimator` | ✅ Pass |
| `test_load_and_healthy_patient` | `NeuralStateEstimator` | ✅ Pass (optional) |
| `test_stressed_patient` | `NeuralStateEstimator` | ✅ Pass (optional) |
| `test_rule_formula_agreement` | `NeuralStateEstimator` | ✅ Pass (optional) |

All tests in the standard suite (`make test`) pass with exit code `0`.

---

## 6. Safety Invariant Analysis

The following invariants are enforced by design. Each is traceable to source code.

### Invariant 1 — Infusion Rate Bounds

**Source:** `src/config_defaults.hpp`, `src/AdaptiveController.cpp`, `src/SafetyMonitor.cpp`

```
MIN_INFUSION_RATE_ML_MIN = 0.1 ml/min
MAX_INFUSION_RATE_ML_MIN = 1.5 ml/min
```

`AdaptiveController::decide` clamps `desired_rate` to `[0.1, profile.max_safe_infusion_rate]`
before forwarding to `SafetyMonitor::evaluate`. The safety monitor then independently caps the
rate via `max_allowed_rate`. The final command can never exceed the lower of these two bounds.

**Verified by:** `test_volume_limit`, `test_cardiac_reserve`

---

### Invariant 2 — Safety Monitor Cannot Be Bypassed

**Source:** `src/AdaptiveController.cpp` (lines 82–88)

```cpp
auto safety_check = safety.evaluate(desired_rate, state, dt_minutes);
if (desired_rate > safety_check.max_allowed_rate) {
    desired_rate = safety_check.max_allowed_rate;
    safety_limited = true;
}
```

The controller always passes its computed rate through `SafetyMonitor::evaluate` and accepts
any reduction imposed by it. There is no code path that skips the safety check.

---

### Invariant 3 — Cardiac Reserve Limiting

**Source:** `src/SafetyMonitor.cpp`

```
MIN_CARDIAC_RESERVE = 0.2
```

When `state.cardiac_reserve < 0.2`, `max_allowed_rate` is capped at `0.5 ml/min` and
`LOW_CARDIAC_RESERVE` is appended to the warning string.

`AdaptiveController::apply_cardiac_limiting` provides a second, independent cardiac scaling
step that reduces the rate before it reaches the safety monitor, providing defence in depth.

**Verified by:** `test_cardiac_reserve`

---

### Invariant 4 — Volume Overload Protection

**Source:** `src/SafetyMonitor.cpp`

```
DAILY_VOLUME_PER_KG_ML = 35.0 ml/kg
```

Maximum 24-hour volume = `weight_kg × 35.0` ml, further reduced by:
- 30% for cardiac conditions (`× 0.7`)
- 40% for renal impairment (`× 0.6`)

When projected volume exceeds 90% of this limit, `max_allowed_rate` is capped at `0.3 ml/min`.

**Verified by:** `test_volume_limit`

---

### Invariant 5 — Rate-of-Change Limiting

**Source:** `src/SafetyMonitor.cpp`, `src/config_defaults.hpp`

```
MAX_RATE_CHANGE_ML_MIN = 0.3 ml/min per control cycle
```

If the requested rate differs from the most recent rate by more than `0.3 ml/min`, the rate
is clamped to `last_rate ± 0.3`. This prevents step changes and oscillation in the infusion
trajectory.

---

### Invariant 6 — High-Risk State Throttling

**Source:** `src/SafetyMonitor.cpp`

```
HIGH_RISK_THRESHOLD = 0.75
```

When `state.risk_score > 0.75`, `max_allowed_rate` is capped at `0.6 ml/min` and
`HIGH_RISK_STATE` is flagged.

---

### Invariant 7 — Tachycardia Detection

**Source:** `src/SafetyMonitor.cpp`

When `state.heart_rate_bpm > profile.baseline_hr_bpm × 1.4`, `max_allowed_rate` is reduced
to `0.4 ml/min` and `TACHYCARDIA_DETECTED` is flagged.

---

### Invariant 8 — Emergency Minimum Rate

**Source:** `src/SafetyMonitor.cpp`

```cpp
if (result.max_allowed_rate < 0.1 && state.hydration_pct < 50.0) {
    result.max_allowed_rate = 0.1;
    warnings << "EMERGENCY_MIN_RATE ";
}
```

If all safety checks together would reduce the rate below `0.1 ml/min`, but the patient is
critically dehydrated (hydration < 50%), the rate is forced back to the minimum safe level.
This prevents total infusion cessation in critical cases.

---

### Invariant 9 — Determinism

**Source:** `src/StateEstimator.cpp`, `src/AdaptiveController.cpp`

All control logic is deterministic. Given identical `Telemetry` and `PatientProfile` inputs,
the system produces identical `ControlOutput`. No random number generators, hardware
timestamps, or non-deterministic operations appear in the control path.

---

### Invariant 10 — Signal Coherence Damping

**Source:** `src/StateEstimator.cpp`

`calculate_coherence` reduces the coherence score when:
- HR is outside 40–180 bpm (×0.5 multiplier)
- Temperature is outside 35–40 °C (×0.7)
- SpO₂ < 85% (×0.6)
- HR variance over 5 samples > 400 bpm² (×0.7)

`apply_coherence_modulation` then scales the desired infusion rate by `coherence_sigma`, so
noisy or degraded telemetry reduces — rather than amplifies — infusion aggressiveness.

---

## 7. Configuration Constants and Bounds

All constants are centralised in `src/config_defaults.hpp`.

### Control Timing

| Constant | Value | Description |
|---|---|---|
| `CONTROL_FREQUENCY_HZ` | 5 | Control loop rate |
| `CONTROL_PERIOD_SEC` | 0.2 s | Period between control ticks |

### Infusion Rate Bounds

| Constant | Value | Description |
|---|---|---|
| `MIN_INFUSION_RATE_ML_MIN` | 0.1 ml/min | Hard lower bound |
| `MAX_INFUSION_RATE_ML_MIN` | 1.5 ml/min | Hard upper bound |

### Risk Thresholds

| Constant | Value | Description |
|---|---|---|
| `LOW_RISK_MAX` | 0.3 | Risk scores ≤ 0.3 are low-risk |
| `MODERATE_RISK_MAX` | 0.6 | Risk scores ≤ 0.6 are moderate |
| `HIGH_RISK_THRESHOLD` | 0.75 | Risk scores > 0.75 trigger throttling |

### Cardiac Safety

| Constant | Value | Description |
|---|---|---|
| `MIN_CARDIAC_RESERVE` | 0.2 | Below this: `LOW_CARDIAC_RESERVE` warning |
| `CARDIAC_LIMIT_THRESHOLD` | 0.3 | Below this: controller applies sigmoid scaling |

### Rate-of-Change Safety

| Constant | Value | Description |
|---|---|---|
| `MAX_RATE_CHANGE_ML_MIN` | 0.3 ml/min | Maximum step change per cycle |

### Volume Safety

| Constant | Value | Description |
|---|---|---|
| `DAILY_VOLUME_PER_KG_ML` | 35.0 ml/kg | Base 24-hour volume limit |

### Prediction

| Constant | Value | Description |
|---|---|---|
| `PREDICTION_HORIZON_MIN` | 10 minutes | Look-ahead window |
| `UNCERTAINTY_GROWTH_PER_MIN` | 0.05 per minute | Confidence decay in predictions |

---

## 8. Failure Mode Analysis

### Sensor Noise or Degraded Signal Quality

**Behaviour:** `calculate_coherence` reduces `coherence_sigma`. `apply_coherence_modulation`
multiplies the desired rate by this value, reducing aggressiveness proportionally.

**Result:** Conservative, dampened response. Infusion does not increase on unreliable data.

---

### Out-of-Range Heart Rate

**Behaviour:** HR outside 40–180 bpm halves the coherence score. If HR exceeds
`1.4 × baseline_hr_bpm`, `TACHYCARDIA_DETECTED` is raised and rate is capped at 0.4 ml/min.

**Result:** Dual protection: coherence damping reduces desired rate; safety monitor caps it.

---

### Low SpO₂

**Behaviour:** SpO₂ < 85% reduces coherence by 40%. SpO₂ < 90% reduces tissue efficiency in
the energy transfer calculation. SpO₂ < 95% reduces cardiac reserve via the reserve formula.

**Result:** Multi-layered response — energy proxy, cardiac reserve, and coherence all degrade,
driving a conservative control trajectory.

---

### Cumulative Volume Approaching Limit

**Behaviour:** `update_volume` accumulates delivered volume. When projected volume exceeds
90% of the daily limit, `VOLUME_LIMIT_APPROACH` fires and rate is capped at 0.3 ml/min.

**Result:** Throttling begins before the hard limit is reached.

---

### Zero Cardiac Reserve

**Behaviour:** `apply_cardiac_limiting` applies a sigmoid scaling factor when `cardiac_reserve
< 0.3`. Below `0.2`, the safety monitor adds `LOW_CARDIAC_RESERVE` and caps at 0.5 ml/min.
Emergency minimum rate (0.1 ml/min) is preserved if the patient is critically dehydrated.

**Result:** Layered protection; rate is reduced but life-sustaining minimum is maintained in
extremis.

---

### Prediction Unavailable

**Behaviour:** `predict_forward` requires at least 5 historical state samples. If fewer are
available, it returns `std::nullopt`. The controller skips the predictive boost when no
prediction is available.

**Result:** System operates on current state alone — conservative and safe.

---

## 9. Known Gaps and Limitations

The following gaps are acknowledged in the v4.0.0 codebase. They do not affect the validity
of the current pre-clinical research scope but are relevant for future development.

| Gap | Detail | Status |
|---|---|---|
| **`AdaptiveController` has no unit tests** | Controller logic (`calculate_base_rate`, `apply_coherence_modulation`, etc.) is exercised only via integration. | Deferred to v4.1 |
| **`SystemLogger` has no unit tests** | NDJSON output format, escape logic, and file I/O are untested in isolation. | Deferred to v4.1 |
| **`RestApiServer` has no unit tests** | JSON serialisation and HTTP response formatting are untested. | Deferred to v4.1 |
| **`simulation_engine.cpp` not in Makefile** | The `SimulationEngine` compiles but has no test target and is not exercised by CI. | Known issue |
| **`PatientProfile` has no default initialisers** | Partially initialised `PatientProfile` objects risk undefined behaviour from uninitialised reads in test code. | Known issue |
| **REST API binds to `0.0.0.0` by default** | No authentication, no TLS, and global network binding are unsafe outside an isolated research network. | Research-only |
| **REST API is single-threaded** | A slow or malformed HTTP connection will stall all subsequent requests. | Acceptable for research use |
| **Whitepaper simulation results not reproducible from this repository** | The multi-patient statistical results cited in the whitepaper require a validated simulation harness not included in this repository. | Documented |

---

## 10. Path to Clinical Validation

This document does **not** constitute clinical validation. The following steps are required
before this system could be considered for regulated clinical use:

1. **Hardware-in-the-loop testing** with certified infusion pumps and sensors
2. **Formal hazard analysis** (e.g., FMEA, FTA) covering all failure modes
3. **Formal verification** of safety-critical invariants (e.g., via TLA+, CBMC, or Frama-C)
4. **Controlled clinical trials** with qualified medical oversight
5. **Regulatory submission** — FDA Class II/III clearance or CE marking under MDR 2017/745
6. **Medical-grade cybersecurity hardening** including authentication, TLS, and audit logging
7. **EHR and clinical workflow integration**
8. **Expanded test coverage** — at minimum, `AdaptiveController`, `SystemLogger`, and end-to-end
   simulation harness

This system is designed as a transparent, auditable research baseline that supports — but does
not replace — those processes.

---

© 2025–2026 Don Michael Feeney Jr.  
Released under the MIT License  
Research use only
