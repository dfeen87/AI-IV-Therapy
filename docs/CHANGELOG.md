# Changelog

All notable changes to this project are documented in this file.

This project follows **Semantic Versioning** and the principles of **Keep a Changelog**.  
All releases are **pre-clinical research only** unless explicitly stated otherwise.

---

## [1.0.0] — Stable Research Reference Release

**Status:** Pre-Clinical Research (Non-Clinical Use)

Version **1.0.0** marks the first *stable, locked reference release* of the AI-Optimized Intravenous Therapy Control System. This release consolidates prior architectural work, formalizes extensibility, and establishes a deterministic, auditable research platform suitable for long-term validation and comparison.

### Added
- Stable, deterministic adaptive IV control core
- Formally defined plugin architecture with guarded lifecycle hooks
- Multi-layer safety enforcement integrated into the control loop
- Deterministic test harness validating:
  - Infusion bounds
  - Risk escalation behavior
  - Cardiac reserve limiting
- Reference HTML dashboard with strict separation of:
  - Telemetry acquisition
  - Control computation
  - Safety enforcement
  - Rendering and instrumentation
- Plugin documentation (`PLUGINS.md`) defining extension contracts and constraints
- Architecture documentation clarifying system boundaries and data flow
- External API surface for integration testing and C++ parity validation

### Notes
- This release is a **research reference**, not a prototype.
- No clinical validation or certified hardware integration is included.
- Backward compatibility is guaranteed for v1.x releases.

---

## [0.3.0] — Configuration & Reproducibility Foundations

This release strengthens scientific rigor and reproducibility while preserving all validated runtime behavior.

### Added
- Centralized configuration defaults  
  - New file: `src/config_defaults.hpp`
  - Canonical definitions for:
    - Control timing
    - Infusion rate bounds
    - Risk thresholds
    - Cardiac safety limits
    - Energy transfer parameters
- Replay and analysis interface (conceptual)  
  - New file: `src/replay_logger.hpp`
  - Defines mechanisms for:
    - Offline replay
    - Auditability
    - Post-run visualization and analysis
- README updates reflecting:
  - Stabilized architecture
  - Configuration and replay foundations
  - Current research scope and non-goals

### Scope & Compatibility
- No functional or behavioral changes
- No modifications to control logic, safety enforcement, or simulation behavior
- Fully backward compatible with v0.2.x

### Notes
This release is intentionally infrastructure-focused and prepares the system for future research without increasing runtime complexity.

---

## [0.2.0] — Architecture & System Contract Stabilization

This release advances the project from an initial research prototype toward a structured, maintainable system architecture.

### Added
- Formal system architecture documentation describing:
  - Control philosophy
  - Estimation, control, and safety separation
  - Predictive control rationale
  - Explicit design non-goals
- Core system data contracts defining stable interfaces for:
  - Patient profiles
  - Telemetry snapshots
  - Derived system state
  - Control command outputs
  - Safety limits

### Scope & Compatibility
- No functional or behavioral changes
- Control logic, safety constraints, and simulation behavior unchanged
- Fully backward compatible with v0.1.0

### Notes
This release establishes architectural clarity and audit readiness without expanding scope.

---

## [0.1.0] — Research Baseline Release

This release marks the initial public research baseline for the AI-Optimized Intravenous Therapy Control System.

### Added
- Real-time adaptive IV infusion controller implemented in C++17
- Simulated biometric inputs:
  - Hydration
  - Heart rate
  - Temperature
  - SpO₂
  - Lactate
  - Fatigue
  - Blood loss indicators
- Nonlinear physiological modeling and predictive trend analysis
- Dynamic infusion rate adjustment within strict safety boundaries
- Multi-layer safety constraints:
  - Volume limits
  - Cardiac reserve protection
  - Rate-of-change control
- Complete audit trail of telemetry, control actions, and safety overrides
- Lightweight HTML dashboard for monitoring and visualization
- Initial research documentation and white paper

### Scope & Limitations
- Research and simulation only
- No FDA clearance or clinical validation
- No certified medical hardware integration
- No machine-learning models trained on patient datasets

### Notes
This release establishes the foundational research platform on which all future versions build.

---

## General Disclaimer

This software is provided **for research purposes only**.  
It must not be used for patient care without appropriate regulatory approval, clinical validation, certified hardware integration, and qualified medical oversight.

© 2025 Don Michael Feeney Jr.  
Released under the MIT License.
