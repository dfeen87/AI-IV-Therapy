# Changelog

All notable changes to this project will be documented in this file.

The format follows **Keep a Changelog** principles and **Semantic Versioning**.

---

## [1.0.0] — Stable Research Reference

### Added
- Stable, deterministic control core with locked v1.0 behavior
- Plugin architecture with guarded lifecycle hooks
- Official plugin API documentation (`PLUGINS.md`)
- Deterministic test harness validating safety bounds and risk escalation
- Session recording, safety clamping, and telemetry logging via plugins
- Explicit external integration surface for C++ parity testing
- Architecture documentation clarifying data flow and system boundaries

### Changed
- Formalized separation between control logic, rendering, and instrumentation
- Promoted HTML dashboard to reference implementation status
- Clarified system intent as a stable pre-clinical research platform

### Stability
- Core control model and plugin API are considered **stable**
- Future changes will preserve backward compatibility unless a major version is released

### Notes
- This release is intended for **research, simulation, and validation**
- Not FDA-cleared and not approved for clinical use
- Clinical deployment requires regulatory approval, certified hardware, and clinical trials

---

## [0.1.0] — Research Baseline

### Added
- Initial adaptive IV therapy control system (C++17)
- Physiological state estimation and predictive control logic
- Multi-layer safety constraints (volume, cardiac reserve, rate limiting)
- Simulation framework for dehydration and metabolic stress
- HTML dashboard for real-time visualization
- Research documentation and white paper

### Notes
- This release is intended for research and simulation only
- No clinical validation or certified hardware integration is included
