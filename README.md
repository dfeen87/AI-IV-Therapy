# AI-Optimized Intravenous Therapy Control System

**License:** MIT  
**Primary Language:** C++17 (with HTML/JS reference implementation)  
**Status:** v1.0.0 — Stable Research Reference (Pre-Clinical)

---

## Overview

This repository contains a **v1.0 stable reference implementation** of an AI-optimized, closed-loop intravenous (IV) therapy control system.

The system is designed for **pre-clinical research, simulation, and algorithm validation** across critical care, space medicine, and extreme survival environments. It demonstrates how real-time physiological telemetry, predictive modeling, and layered safety constraints can be combined into a deterministic, auditable control architecture.

This release establishes a **locked core model and plugin extension API** suitable for further research, C++ reimplementation, and integration testing.

---

## Problem Statement

Conventional IV therapy relies on static infusion protocols that do not adapt to rapidly changing patient physiology. In high-risk scenarios, this rigidity can result in:

- Fluid overload or under-resuscitation  
- Delayed response to physiological deterioration  
- Inefficient cellular energy (ATP) recovery  
- Increased risk of preventable complications  

---

## Solution

An AI-powered, closed-loop IV control framework that:

- Continuously estimates patient state from multimodal telemetry  
- Predicts near-term physiological trajectories  
- Dynamically adapts infusion rates within strict safety bounds  
- Optimizes metabolic recovery and cellular energy transfer  
- Separates control logic from visualization and instrumentation  

---

## Key Features

### 1. Multi-Modal State Estimation

- Nonlinear energy-transfer modeling using sigmoid hydration curves  
- Exponential penalties for metabolic stress and lactate accumulation  
- Cardiac reserve estimation with age-adjusted physiological limits  
- Composite risk scoring across dehydration, hypoxia, and thermal stress  
- Signal coherence metrics to detect and mitigate sensor noise  

### 2. Predictive Control

- Forward state prediction with uncertainty awareness  
- Rolling-window trend analysis  
- Proactive intervention before critical thresholds are reached  

### 3. Multi-Layer Safety System

- Infusion-rate bounding (0.1–1.5 ml/min)  
- Cardiac reserve–based throttling  
- Rate-of-change limiting to prevent oscillations  
- Emergency fallback logic with minimum safe infusion guarantees  

### 4. Adaptive AI Control

- Risk-amplified demand modeling for acute scenarios  
- Coherence-modulated control gain under noisy conditions  
- Cardiac reserve feedback to protect cardiovascular function  
- Smooth infusion trajectories for stability and comfort  

### 5. Plugin-Based Hardening (v1.0)

- Explicit lifecycle hooks for instrumentation and safety logic  
- Safe, isolated plugin execution  
- Deterministic core preserved under extension  
- Examples include logging, session recording, and safety clamping  

---

## Application Domains

| Domain | Use Case | Impact |
|--------|----------|--------|
| Critical Care | Dynamic ICU fluid management | Reduced complications, faster recovery |
| Space Medicine | Astronaut hydration & fatigue control | Mission safety and performance |
| Military & Disaster Relief | Wearable-integrated field IV systems | Improved survival in austere settings |
| Extreme Environments | Post-exertion metabolic recovery | Optimized hydration and ATP replenishment |
| Emergency Medicine | Pre-hospital resuscitation | Continuous adaptation during transport |

---

## System Data Flow

```
Wearable Sensors
(Hydration, Heart Rate, SpO₂, Temperature, Lactate, Fatigue)
          ↓
State Estimator
  - Signal fusion and coherence checks
  - Nonlinear energy modeling
  - Cardiac reserve and risk scoring
  - Predictive trajectory estimation
          ↓
Adaptive AI Controller
  - Risk-amplified demand modeling
  - Coherence-modulated control gain
  - Multi-objective optimization
          ↓
Safety Monitor
  - Volume limits
  - Cardiac load protection
  - Rate-of-change constraints
  - Emergency overrides
          ↓
Infusion Pump Interface
(0.1–1.5 ml/min, real-time control)
```

---

## Installation and Compilation

### Prerequisites

- C++17-compatible compiler (GCC ≥7, Clang ≥5, MSVC ≥2017)
- POSIX threads (pthread)

### Build Instructions

```bash
git clone https://github.com/dfeen87/ai-iv-therapy.git
cd ai-iv-therapy-system
g++ -std=c++17 -pthread -O2 ai_adaptive_iv_therapy_system.cpp -o ai_iv_system
./ai_iv_system
```

### Output Files

- `ai_iv_[timestamp]_system.log` — system events and warnings  
- `ai_iv_[timestamp]_telemetry.csv` — raw sensor data  
- `ai_iv_[timestamp]_control.csv` — infusion commands and control rationale  

---

## Simulation and Testing

The default simulation models a **75 kg, 35-year-old patient** undergoing progressive dehydration and metabolic stress.

Simulation scenarios can be modified in the telemetry acquisition layer to emulate:

- Severe dehydration  
- Hemorrhage  
- Hypothermia  
- Compensatory tachycardia or bradycardia  

A deterministic test harness validates system bounds, risk escalation behavior, and cardiac reserve limiting.

---

## Research Status and Validation

**Current Status:** Stable pre-clinical research reference (v1.0)

Clinical deployment requires:

- Regulatory clearance (FDA Class II/III, CE)  
- Controlled clinical trials  
- Integration with FDA-cleared infusion pumps and sensors  
- Medical-grade cybersecurity hardening  
- EHR and clinical workflow integration  

---

## Development Roadmap (Post–v1.0)

- Advanced state estimation (Kalman and Bayesian filtering)  
- Controller tuning using ICU datasets  
- Formal C++ parity implementation  
- Hardware abstraction layer for infusion pumps  
- Wearable sensor integration (Bluetooth Low Energy)  
- Multi-patient simulation framework  
- Regulatory documentation package  

---

## Scientific Foundations

This system builds on established research in:

- Closed-loop fluid resuscitation  
- Predictive hemodynamic monitoring  
- Wearable biosensor fusion  
- Cellular energy transfer and ATP recovery  
- Space medicine fluid balance research  

All mathematical models and control laws are documented inline for auditability and reproducibility.

---

## Contributing

This is an open research platform.

Contributions are welcome in:

- Control theory and machine learning  
- Safety and fault-tolerant systems  
- Sensor and pump integration  
- Clinical validation studies  
- Visualization and analysis tooling  

---

## License

MIT License — open for research, modification, and clinical translation.

© 2025 Don Michael Feeney Jr.
