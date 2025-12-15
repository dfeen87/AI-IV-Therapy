# System Architecture — AI-Optimized Intravenous Therapy Control System

## Purpose of This Document

This document describes the architectural design, system boundaries, and control philosophy of the AI-Optimized Intravenous Therapy Control System.

It is intended to:
- Clarify how the system is structured and why
- Support research review and collaboration
- Enable safe future extension without architectural drift
- Separate validated behavior from exploratory components

This document does **not** define clinical deployment requirements or regulatory approval status.

---

## Architectural Design Principles

The system is designed around five core principles:

1. **Closed-Loop Control with Explicit Safety Separation**
2. **Physiological Interpretability Over Black-Box Optimization**
3. **Predictive, Not Reactive, Decision-Making**
4. **Graceful Degradation Under Uncertainty**
5. **Clear Boundaries Between Estimation, Control, and Actuation**

These principles guide all architectural decisions.

---

## High-Level System Decomposition

The system is composed of five primary subsystems:

[ Sensors ]
↓
[ State Estimator ]
↓
[ Predictive Model ]
↓
[ Adaptive Controller ]
↓
[ Safety Monitor ]
↓
[ Infusion Actuator ]


Each subsystem is logically isolated and may be independently validated.

---

## 1. Sensor Interface Layer

### Role
The sensor layer represents the ingestion boundary for physiological data.

### Inputs (Simulated in v0.1.x)
- Hydration percentage
- Heart rate
- Oxygen saturation (SpO₂)
- Core temperature
- Lactate concentration
- Fatigue index
- Blood loss proxy

### Design Notes
- Sensor values are treated as **noisy, imperfect estimates**
- No single sensor is trusted in isolation
- Future hardware integration is explicitly deferred

---

## 2. State Estimation Layer

### Role
Transforms raw sensor signals into a coherent physiological state estimate.

### Responsibilities
- Signal coherence validation
- Nonlinear normalization and scaling
- Composite metric construction
- Cardiac reserve estimation
- Energy transfer proxy calculation

### Design Rationale
This layer exists to:
- Prevent control logic from operating directly on raw data
- Preserve interpretability of derived variables
- Enable probabilistic extensions (e.g., Kalman filtering)

The estimator is intentionally deterministic in v0.1–v0.2 to support traceability.

---

## 3. Predictive Modeling Layer

### Role
Projects short-horizon physiological trajectories.

### Characteristics
- Rolling-window trend analysis
- Near-term prediction (minutes, not hours)
- Conservative extrapolation
- Explicit uncertainty tolerance

### Key Insight
Prediction is used to **shape control behavior**, not to maximize performance.
This prevents aggressive actions driven by speculative futures.

---

## 4. Adaptive Control Layer

### Role
Computes the desired infusion rate based on estimated and predicted state.

### Control Inputs
- Hydration deficit
- Energy gap
- Metabolic load
- Risk amplification factor
- Signal coherence
- Cardiac reserve feedback

### Control Philosophy
The controller is:
- **Risk-amplifying**, not risk-seeking
- Smooth and rate-limited
- Biased toward physiological stability
- Explicitly bounded

No learning-based policy optimization is used at this stage.

---

## 5. Safety Monitor Layer

### Role
Acts as an independent enforcement layer that may override control output.

### Safety Constraints
- Maximum infusion rate
- Rate-of-change limits
- Cardiac reserve protection
- Volume accumulation limits
- Emergency fallback behavior

### Architectural Rule
> The safety layer cannot be bypassed by the controller.

This separation is intentional and non-negotiable.

---

## Infusion Actuation Interface

### Role
Represents the final command interface to an infusion device.

### Characteristics
- Accepts only validated, bounded rates
- Operates in real time
- Assumes actuator compliance is imperfect

Hardware integration is outside the scope of current releases.

---

## Visualization & Dashboard Layer

### Purpose
The dashboard is a **research visualization tool**, not a clinical UI.

It exists to:
- Expose internal system state
- Demonstrate control behavior
- Support debugging and explanation
- Align visual outputs with mathematical variables

The dashboard mirrors the internal architecture rather than abstracting it.

---

## Auditability & Traceability

All control decisions are designed to be:
- Logged
- Reconstructable
- Interpretable

This is a prerequisite for:
- Research validation
- Clinical translation
- Regulatory reasoning

---

## Non-Goals (Explicit)

The following are **intentionally excluded** at this stage:
- Autonomous clinical decision authority
- End-to-end machine learning control
- Real patient data ingestion
- Regulatory compliance claims

Excluding these is a design decision, not a limitation.

---

## Future Architectural Extensions

Planned extensions that fit within this architecture:
- Probabilistic state estimation
- Hardware abstraction layers
- Multi-patient simulation
- Learning-based tuning within safety envelopes
- Formal verification of safety constraints

The current architecture is designed to support these without restructuring.

---

## Summary

This system is architected as a **conservative, interpretable, safety-first control framework** intended for research into adaptive intravenous therapy.

Its structure prioritizes:
- Clarity over complexity
- Safety over performance
- Predictability over novelty

This foundation enables responsible evolution toward future clinical and extreme-environment applications.
