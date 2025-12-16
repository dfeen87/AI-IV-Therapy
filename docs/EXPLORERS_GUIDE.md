# Explorer’s Guide  
AI-Optimized Intravenous Therapy Control System

This guide is intended for researchers, engineers, and clinicians who are
exploring this repository for the first time.

It explains **what to read first**, **how the system fits together**, and
**what this project is (and is not)**.

---

## 1. What This Project Is

This repository contains a **research-grade, closed-loop control system**
for adaptive intravenous (IV) therapy.

The system explores how:
- real-time physiological signals
- predictive modeling
- energy-transfer-aware control
- and layered safety constraints

can be combined into a **conservative, interpretable, and extensible**
control architecture.

This is a **simulation and research platform**, not a clinical product.

---

## 2. What This Project Is Not

To set expectations clearly:

- ❌ Not an FDA-cleared medical device
- ❌ Not an autonomous clinical decision system
- ❌ Not trained on real patient data
- ❌ Not a black-box AI controller

These exclusions are **intentional design decisions**, not limitations.

---

## 3. Recommended Reading Order

If you are new to the project, follow this order:

### Step 1 — High-Level Overview
📄 `README.md`

Start here to understand:
- the problem being addressed
- the system’s scope
- safety and ethical boundaries

---

### Step 2 — System Architecture
📄 `docs/ARCHITECTURE.md`

This explains:
- how the system is decomposed
- why safety is separated from control
- how prediction is used conservatively
- how future extensions fit without redesign

If you only read one deep document, read this.

---

### Step 3 — Core System Code
📄 `src/adaptive_iv_therapy_control_system.cpp`

Focus on these sections:
- **StateEstimator** — how physiological state is derived
- **AdaptiveController** — how infusion decisions are made
- **SafetyMonitor** — how limits are enforced
- **AIIVSystem** — how everything is orchestrated

The code is intentionally verbose and traceable.

---

### Step 4 — System Data Contracts
📄 `src/iv_system_types.hpp`

This file defines:
- telemetry snapshots
- patient profiles
- system state
- control outputs
- safety limits

It represents the **stable interface layer** of the system.

---

### Step 5 — Visualization
📄 `dashboard/ai_iv_dashboard.html`

This dashboard:
- mirrors internal system variables
- visualizes energy transfer, risk, and control
- is designed for explanation and debugging

It is a research visualization, not a clinical UI.

---

## 4. Understanding the Energy Transfer Model

The system implements a full nonlinear energy transfer model
described in the accompanying white paper (Section 4.1).

Key ideas:
- Energy delivery is flow-dependent
- Tissue absorption is perfusion- and oxygen-sensitive
- Velocity optimization follows a Gaussian efficiency curve
- Control decisions balance hydration, energy, and safety

Both normalized (`E_T`) and absolute (`W/kg`) energy metrics are exposed.

---

## 5. How to Run and Explore

1. Compile and run the system
2. Observe console output and log files
3. Open the HTML dashboard
4. Watch how control decisions evolve over time
5. Inspect the CSV logs for auditability

No external dependencies or hardware are required.

---

## 6. Where Contributions Fit Best

Contributions are most valuable in:
- state estimation improvements
- noise and uncertainty modeling
- safety constraint refinement
- visualization clarity
- simulation realism

Feature expansion should preserve the existing safety-first architecture.

---

## 7. Final Notes

This project values:
- clarity over cleverness
- safety over performance
- interpretability over opacity
- disciplined scope over rapid expansion

If you are exploring this repository, you are encouraged to read,
run, question, and learn from it.

---

*Thank you for taking the time to explore this work.*
