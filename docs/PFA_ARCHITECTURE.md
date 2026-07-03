# Precision-Flow Architecture (PFA)

## Conceptual Overview
The Precision-Flow Architecture (PFA) marries deterministic, constraint-based validation with adaptive, biological-inspired dosing logic. It establishes a "machine-grade spine" that rigorously filters physiological signals before they reach the higher-level "circulatory system" of AI-IV-Therapy.

### Inspiration: CuraFrame
PFA draws direct conceptual inspiration from **CuraFrame**, a scientific reasoning framework designed for safety-first therapeutic candidate evaluation. Where CuraFrame acts as a falsification engine to enforce pharmacokinetic (PK) and pharmacodynamic (PD) constraints off-line, PFA operates in real-time, enforcing constraints dynamically against incoming telemetry data.

The core tenets adapted from CuraFrame include:
1. **Safety before speed**: Hard constraints prevent unsafe conditions regardless of adaptive intent.
2. **Deterministic reasoning**: Every state transformation is entirely reproducible and auditable.
3. **Explicit constraints**: Noise and out-of-bounds telemetry are surgically rejected.
4. **No "surprise" behavior**: The adaptive controller operates solely on validated, clean data.

## Architecture

The PFA consists of three primary conceptual layers:

1. **Install-Bridge (Sensory Ingestion):**
   Acquires raw telemetry and generates the initial `PatientState`.
2. **Precision Spine (The Spine):**
   A strict, deterministic routing and validation layer. It guarantees that all states passed up the chain are physiologically viable and structurally sound.
3. **Adaptive Controller (The Circulatory System):**
   The flow-based treatment engine that modulates dosing based on the validated, clean state.

### The Precision Spine Module

The `precision_spine` module acts as the gatekeeper. All incoming `PatientState` flows must pass through this module before reaching the `AdaptiveController`.

#### Key Primitives

* **`TreatmentFlow`**:
  The semantic and numeric representation of treatment signals moving through the system. Defined as an `alignas(64)` struct, it ensures all routing operations use aligned, deterministic memory layouts.
* **`dose_route()`**:
  Deterministically routes treatment signals, performing baseline validation on constraints like hydration bounds.
* **`reject_noise()`**:
  Suppresses low-confidence fluctuations and filters out impossible physiological states (noise artifacts).
* **`fallback_floor()`**:
  Enforces stability under noisy conditions. If a flow is invalid or highly unstable, it forces physiological variables to a conservative baseline, preventing panic cascades.
* **`align_buffer()`**:
  Guarantees 64-byte alignment for buffers of `TreatmentFlow` objects, ensuring memory predictability and strict alignment for all deterministic flows.

## Operational Flow

```
[ Raw Telemetry ] -> StateEstimator -> [ Raw PatientState ]
                                              |
                                              v
                                   precision_spine::dose_route()
                                              |
                                              v
                                 precision_spine::reject_noise()
                                              |
                                              v
                                precision_spine::fallback_floor()
                                              |
                                              v
                                 [ Validated PatientState ]
                                              |
                                              v
                                  AdaptiveController::decide()
                                              |
                                              v
                                      [ ControlOutput ]
```

## Benefits
* **No brittle edges**: Explicit bounds checking prevents edge-case crashes.
* **No panic cascades**: The `fallback_floor` ensures the adaptive controller never receives wild, unstable input that could lead to drastic over-corrections.
* **No allocator surprises**: Memory allocation relies on predictable, stack-friendly or pre-allocated aligned arrays.
* **Predictable fallback behavior**: When signals degrade, the system naturally degrades to a safe, conservative state.
* **Surgical routing**: Constraints are applied precisely, allowing the adaptive layer to focus purely on therapeutic optimization.