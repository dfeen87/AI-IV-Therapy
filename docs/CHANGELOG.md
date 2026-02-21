# Changelog

All notable changes to this project are documented in this file.

This project follows **Semantic Versioning** and the principles of **Keep a Changelog**.  
All releases are **pre-clinical research only** unless explicitly stated otherwise.

---

## [4.0.0] — Full Modular Refactor, REST API & Observability Platform

**Status:** Pre-Clinical Research (Non-Clinical Use)

Version **4.0.0** is a ground-up architectural overhaul of the AI-Optimized Intravenous Therapy
Control System. Every subsystem was decomposed into a standalone, independently compilable module.
This release establishes a stable, auditable foundation for future research, integration, and
clinical translation work.

---

### Added

#### Core Architecture
- Decomposed monolithic control system into five distinct compilation units:
  - `StateEstimator` — signal fusion, coherence scoring, energy proxy, cardiac reserve, risk scoring
  - `AdaptiveController` — demand modeling, coherence modulation, cardiac limiting, predictive boost
  - `SafetyMonitor` — volume limits, cardiac load, rate-of-change, emergency overrides
  - `SystemLogger` — structured NDJSON alerts, telemetry CSV, control CSV, flush-safe I/O
  - `SimulationEngine` — parameterized physiological scenario driver (sinusoidal baseline)
- Centralized system data contracts in `iv_system_types.hpp` (stable, versioned interface layer)
- Centralized tunable parameters in `config_defaults.hpp`; `SafetyMonitor` now references these
  constants directly instead of maintaining duplicates
- `replay_logger.hpp` — offline replay and post-run auditability scaffolding

#### Observability & Alerting
- Structured NDJSON alert events emitted inline to the existing `*_system.log` stream
- Stable alert schema: `timestamp`, `severity`, `source`, `code`, `message`, `context`
- Full JSON character escaping in `SystemLogger` (`escape_json_string`)
- Severity-based immediate flush for `CRITICAL` events; batched flush otherwise
- Compatible with PagerDuty, Opsgenie, ELK, Datadog log ingestion pipelines
- Alert codes cross-referenced in `docs/alerts.md`

#### REST API (Optional)
- Read-only HTTP/1.1 server (`src/rest_api_server.cpp`) enabled with `-DENABLE_REST_API`
- Endpoints: `/api/status`, `/api/telemetry`, `/api/telemetry/history`, `/api/control`,
  `/api/state`, `/api/alerts`, `/api/config`
- Thread-safe data access via `std::mutex`; zero impact on control loop timing
- Global network binding (`0.0.0.0:8080`) for remote access
- Last 1000 telemetry samples buffered in-process
- Full REST API documentation in `docs/REST_API.md`

#### Dashboard
- HTML/JavaScript research dashboard (`dashboard/ai_iv_dashboard.html`)
  - Real-time visualization of all state estimator outputs and control decisions
  - Standalone simulation mode (no server required)
  - REST API integration mode (reads from C++ backend when available)
  - Responsive layout with WCAG AA colour contrast

#### CI / Build
- GitHub Actions CI pipeline (`.github/workflows/ci.yml`):
  - Strict warning build (`-Wall -Wextra -Wpedantic`)
  - Alert emission smoke test (`-DAI_IV_ALERT_LOG_TEST`)
  - REST API build verification
  - **Unit test compilation and execution** (Safety Monitor, State Estimator)
- `Makefile` with `all`, `test`, and `clean` targets

#### Tests
- `tests/test_safety_monitor.cpp` — volume limit and cardiac reserve enforcement
- `tests/test_state_estimator.cpp` — basic estimation correctness

#### Documentation
- `docs/ARCHITECTURE.md` — system decomposition and design rationale
- `docs/CHANGELOG.md` — this file
- `docs/Dashboard.md` — dashboard component guide
- `docs/EXPLORERS_GUIDE.md` — recommended reading order for new contributors
- `docs/PLUGINS.md` — extension contract and lifecycle hooks (JavaScript dashboard layer)
- `docs/REST_API.md` — full endpoint reference with examples
- `docs/ROADMAP.md` — near-, mid-, and long-term research directions
- `docs/VALIDATION.md` — validation scope, system invariants, and out-of-scope guarantees
- `docs/alerts.md` — NDJSON alert line format and filtering reference
- `docs/whitepapers/AI_Optimized_IV_Treatment_White_Paper.md` — technical and clinical overview
- `examples/rest_api_client.py` — Python client demonstrating API integration

---

### Fixed

- **Cardiac reserve formula** (critical correctness bug): `calculate_cardiac_reserve` previously
  derived an age estimate from `baseline_hr_bpm` using `(220 - baseline_hr) / 0.7`, producing
  HRmax ≈ 5.7 bpm for a typical adult. Cardiac reserve was therefore always approximately zero,
  permanently triggering safety throttling and cardiac limiting. Fixed to use `profile.age_years`
  directly: `HRmax = 220 - age_years` (standard Tanaka approximation).
- **`simulation_engine.cpp` field name**: referenced `m.temperature_c` (compilation error);
  corrected to `m.temp_celsius` to match `Telemetry` struct definition.

---

### Critique of This Release

The following is an honest technical assessment of what this release achieves and where it still
falls short. It is written to support future contributors and clinical reviewers.

#### Strengths

1. **Clean modular decomposition.** Separating `StateEstimator`, `AdaptiveController`, and
   `SafetyMonitor` into distinct compilation units with clear interfaces is the right structural
   decision. Each layer is independently testable and auditable.

2. **Safety separation is architectural, not accidental.** The `SafetyMonitor` is an independent
   enforcement layer that the controller cannot bypass. This is the correct safety-first pattern
   for pre-clinical work.

3. **Structured alert schema.** The NDJSON alert format with stable codes, severity levels, and
   optional context fields is immediately useful for integration with real observability pipelines
   without requiring any changes to the log ingestion infrastructure.

4. **Honest design documentation.** `ARCHITECTURE.md`, `VALIDATION.md`, and `EXPLORERS_GUIDE.md`
   explicitly state what the system does *not* do (no machine learning, no real patient data, no
   clinical authority). This is more honest than most research prototypes.

5. **Centralized config.** `config_defaults.hpp` is the right place for tunable constants.

#### Issues and Gaps

1. **"AI" branding overstates what the system currently does.**
   The project is named "AI-Optimized" and the README refers to "Adaptive AI Control Logic".
   The actual implementation is a deterministic, rule-based nonlinear controller — sigmoid
   activation functions, Gaussian velocity efficiency, weighted composite scoring. There is no
   machine learning, no trained model, and no learned policy. The `ARCHITECTURE.md` correctly
   states "No learning-based policy optimization is used at this stage", but this disclaimer is
   buried. The README should reflect it prominently.

2. **The whitepaper simulation results cannot be reproduced from this repository.**
   Section 7 of the whitepaper presents detailed comparative results — 1000 virtual patients,
   5000 simulation runs, p<0.001 statistics, "46% faster recovery", "49% lower cardiac stress".
   The `SimulationEngine` in the repository generates sinusoidal telemetry for a single virtual
   patient; it cannot produce these scenarios. These results are design-study projections, not
   validated experimental outputs. They should be explicitly labelled as such in the whitepaper.

3. **The whitepaper describes ML/TensorFlow infrastructure as if it exists today.**
   Section 2.2.2 lists "TensorFlow Lite: Neural network inference in <10ms" as a current
   technological enabler. Section 9.2 shows a firmware stack with "TensorFlow Lite (ML inference)"
   in the middleware layer. Both describe aspirational future architecture. Section 10.2 correctly
   frames Deep Reinforcement Learning and CNN sensor fusion as *future work*, but the earlier
   sections do not make this temporal distinction clear.

4. **REST API has no authentication and binds to all interfaces by default.**
   The default configuration is `0.0.0.0:8080` with no API key, token, or TLS. The documentation
   acknowledges this but the default itself is unsafe for any networked environment, particularly
   one adjacent to medical infrastructure. The default bind address should be `127.0.0.1`.

5. **REST API is single-threaded.**
   The `server_loop` handles one HTTP client at a time. A slow or malformed connection will stall
   all subsequent requests. For research use this is acceptable; it must be fixed before any
   clinical or networked deployment.

6. **Unit tests are sparse and not yet in CI (fixed in this release).**
   Only `SafetyMonitor` and `StateEstimator` have tests. `AdaptiveController`, `SystemLogger`,
   and `RestApiServer` have no coverage. CI previously built the tests but did not run them.
   This release wires them into the CI pipeline; coverage expansion is deferred to v4.1.

7. **`PatientProfile` members have no default values.**
   Fields such as `weight_kg`, `age_years`, and `baseline_hr_bpm` are declared without
   default member initialisers. Tests that partially initialise a `PatientProfile` stack
   object risk undefined behaviour from uninitialised reads.

8. **`PLUGINS.md` documents a JavaScript-only API.**
   The C++ control system has no plugin hooks. The plugin interface exists only in the HTML
   dashboard. This is architecturally valid (the two are separate systems), but the document
   should state this clearly to avoid the impression that the C++ core is extensible via plugins.

9. **Version numbers are scattered and inconsistent across the codebase.**
   The README badge shows v2.2.0. The startup banner in `main()` prints "v2.0". The REST API
   root endpoint returns `"version":"1.0.0"`. The previous CHANGELOG stopped at v1.0.0. All
   of these should resolve to a single authoritative version string.

10. **`simulation_engine.cpp` is not compiled by CI or `Makefile`.**
    It is an orphaned file with a field name bug (fixed above). Until it is integrated into
    a supported build target it provides no value and will accumulate further drift from the
    main types.

---

### Notes

- This release is a **pre-clinical research reference**, not a clinical product.
- No clinical validation, certified hardware integration, or regulatory submission is included.
- The cardiac reserve formula fix (item 1 under Fixed) changes runtime behaviour: the system
  will no longer permanently operate in cardiac-limited mode. Behaviour under the corrected
  formula should be validated against expected physiological ranges before proceeding with
  further development.
- All simulation results cited in the whitepaper are design-study projections and must be
  reproduced by a validated simulation harness before being cited in clinical or regulatory
  submissions.

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
