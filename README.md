# AI-Optimized Intravenous Therapy Control System (AI-IV)

![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)
![Language: C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Status: Pre-Clinical](https://img.shields.io/badge/Status-Pre--Clinical-orange.svg)
![Version](https://img.shields.io/badge/version-v2.2.0-blueviolet)

**License:** MIT  
**Primary Language:** C++17  
**Supplementary Reference:** HTML / JavaScript (conceptual parity)  
**Current Status:** v2.2.0 — Stable Pre-Clinical Research System  

---

## Overview

This repository contains a **production-grade, pre-clinical reference implementation** of an **AI-optimized, closed-loop intravenous (IV) therapy control system**.

AI-IV demonstrates how **real-time physiological telemetry**, **predictive state modeling**, **deterministic control**, and **layered safety constraints** can be composed into a **fully auditable, safety-first control architecture** suitable for:

- Critical care research
- Space and extreme-environment medicine
- Simulation and algorithm validation
- Future regulated medical device development

> **Important:**  
> This software is **not a clinical device**. It is intended exclusively for **research, simulation, and pre-clinical development**.

---

## Design Philosophy

AI-IV is built around four non-negotiable principles:

1. **Determinism First**  
   Control logic must remain predictable, bounded, and timing-safe.

2. **Safety Is Structural**  
   Safety constraints are embedded into the control architecture—not bolted on.

3. **Observability Without Coupling**  
   Monitoring, alerts, and instrumentation must never influence control decisions.

4. **Auditability by Construction**  
   Every decision, constraint, and fallback is explainable and logged.

---

## Problem Statement

Traditional IV therapy relies on **static infusion protocols** that fail to adapt to rapidly changing patient physiology.

In high-risk or resource-limited environments, this can lead to:

- Fluid overload or under-resuscitation  
- Delayed recognition of physiological deterioration  
- Inefficient cellular energy (ATP) recovery  
- Increased risk of preventable complications  

---

## Solution

AI-IV implements an **adaptive, closed-loop IV control framework** that:

- Continuously estimates patient state from multimodal telemetry
- Predicts near-term physiological trajectories
- Dynamically adjusts infusion rates within strict safety envelopes
- Optimizes metabolic recovery and cellular energy transfer
- Separates control logic from visualization, logging, and alerting

---

## Key Capabilities

### 1. Multi-Modal State Estimation

- Nonlinear hydration and energy-transfer modeling
- Exponential penalties for metabolic stress and lactate accumulation
- Cardiac reserve estimation with physiologically bounded limits
- Composite risk scoring (hydration, oxygenation, thermal load)
- Signal coherence checks to detect sensor degradation or noise

---

### 2. Predictive Control

- Forward state prediction with bounded extrapolation
- Rolling-window trend analysis
- Early intervention before threshold violations occur

---

### 3. Multi-Layer Safety System

- Hard infusion bounds (e.g. 0.1–1.5 ml/min)
- Cardiac reserve-based throttling
- Rate-of-change limiting to prevent oscillation
- Emergency fallback logic with minimum safe infusion guarantees

---

### 4. Adaptive AI Control Logic

- Risk-amplified demand modeling for acute conditions
- Coherence-modulated control gain under noisy telemetry
- Cardiovascular protection via reserve-aware feedback
- Smooth, monotonic infusion trajectories

---

### 5. Production-Safe Observability & Alarm Sync (v2.2.0)

- Structured, machine-ingestable **ALERT NDJSON events**
- Emitted into existing `*_system.log` streams (log-based ingestion)
- Stable alert schema (severity, code, source, message, timestamp)
- No network I/O, no timing impact, no control coupling
- Compatible with PagerDuty, Opsgenie, ELK, Datadog, etc.

> Alerts are **observational only** — they never influence control behavior.

---

### 6. REST API for Global Network Access (Optional)

- Real-time HTTP API for remote monitoring and telemetry access
- JSON endpoints for status, telemetry, control state, and alerts
- Thread-safe, non-blocking operation (zero impact on control loop)
- Read-only design (GET endpoints only for safety)
- Global network binding (0.0.0.0:8080) for remote access
- Historical data buffering (last 1000 telemetry samples)
- CORS-enabled for web dashboard integration

> Enabled with `-DENABLE_REST_API` compile flag. See [REST API Documentation](docs/REST_API.md).

---

## System Architecture

```

Wearable / Simulated Sensors
(Hydration, HR, SpO₂, Temp, Lactate, Fatigue)
↓
State Estimator

* Signal fusion & coherence checks
* Nonlinear energy modeling
* Cardiac reserve estimation
* Risk scoring & prediction
  ↓
  Adaptive AI Controller
* Risk-amplified demand modeling
* Coherence-aware control gain
* Predictive rate adjustment
  ↓
  Safety Monitor
* Volume limits
* Cardiac load protection
* Rate-of-change constraints
* Emergency overrides
  ↓
  Infusion Pump Interface
  (Deterministic, bounded output)

````

---

## Installation & Compilation

### Prerequisites

- C++17-compatible compiler  
  - GCC ≥ 7  
  - Clang ≥ 5  
  - MSVC ≥ 2017  
- POSIX threads (`pthread`)

### Build

**Standard Build:**
```bash
git clone https://github.com/dfeen87/ai-iv-therapy.git
cd ai-iv-therapy
g++ -std=c++17 -O2 -pthread src/adaptive_iv_therapy_control_system.cpp -o ai_iv
./ai_iv
```

**Build with REST API (Global Network Access):**
```bash
g++ -std=c++17 -O2 -pthread -DENABLE_REST_API \
    src/adaptive_iv_therapy_control_system.cpp \
    src/rest_api_server.cpp \
    -o ai_iv
./ai_iv
```

When built with REST API support, the system exposes real-time telemetry, state, and control data via HTTP on port 8080. See [REST API Documentation](docs/REST_API.md) for details.

---

## Continuous Integration

AI-IV uses a minimal, safety-aligned CI gate that focuses on deterministic compilation and an alerting smoke test. The workflow:

* Builds the core system with strict warnings (`-Wall -Wextra -Wpedantic`) under C++17.
* Builds and runs the alert/logging smoke-test variant (`-DAI_IV_ALERT_LOG_TEST`) to verify alert emission paths compile and execute.

CI intentionally does **not** perform performance benchmarks, hardware-in-the-loop checks, network-based validation, or timing-dependent assertions. This scope is appropriate for a safety-critical, pre-clinical system because it verifies build integrity and observability paths without introducing nondeterminism or environmental coupling that could mask control-loop determinism and safety invariants.

---

## Runtime Artifacts

AI-IV produces deterministic, append-only artifacts:

* `ai_iv_[timestamp]_system.log`
  System events, warnings, and structured ALERT lines

* `ai_iv_[timestamp]_telemetry.csv`
  Raw physiological telemetry

* `ai_iv_[timestamp]_control.csv`
  Infusion decisions and control rationale

---

## Simulation & Testing

Default simulation models a **75 kg, 35-year-old subject** undergoing progressive dehydration and metabolic stress.

Scenarios can be modified to emulate:

* Severe dehydration
* Hemorrhage
* Hypothermia
* Tachycardia or bradycardia
* Sensor degradation or dropout

A deterministic harness validates:

* Safety bounds
* Rate-limiting behavior
* Emergency fallback engagement
* Alert emission paths

---

## Research & Regulatory Status

**Current Status:**
Stable, pre-clinical research and simulation system.

Clinical deployment would require:

* FDA Class II / III clearance or CE marking
* Controlled clinical trials
* Integration with certified infusion pumps and sensors
* Medical-grade cybersecurity hardening
* EHR and clinical workflow integration

---

## Roadmap

* Bayesian / Kalman-based state estimation
* Controller tuning using ICU datasets
* Hardware abstraction for pump vendors
* BLE wearable sensor integration
* Multi-patient simulation
* Formal verification of safety invariants
* Optional real-time alert streaming (WebSocket) layer
* Regulatory documentation package

---

## Scientific Foundations

AI-IV builds on established work in:

* Closed-loop fluid resuscitation
* Predictive hemodynamic monitoring
* Wearable biosensor fusion
* Cellular energy transfer & ATP recovery
* Space medicine fluid balance research

All equations and control laws are documented inline for auditability.

---

## Contributing

AI-IV is an **open research platform**.

Contributions are welcome in:

* Control theory & ML
* Safety-critical systems
* Sensor & pump integration
* Simulation & validation
* Visualization & analysis tooling

---

## License

MIT License — open for research, modification, and future clinical translation.

© 2025–2026 Don Michael Feeney Jr.
