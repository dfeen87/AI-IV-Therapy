# AILEE Trust Layer Integration (Simulation Extensions)

**Version:** v4.2.0

> **Staging Notice:** The modules described in this document (`iv_logic/` and
> `iv_extensions/`) are staging additions. They are not yet included in the CI build
> or Makefile targets. Integration into the main simulation loop is deferred to a
> future release.

This document describes the optional plugins and extensions added to the AI-IV-Treatment simulation to model high-fidelity, safety-critical decision validation using the AILEE (Adaptive Integrity Layer for AI Decision Systems) architecture.

**Note:** These extensions are for simulation realism only. They enhance the complexity and behavior of the simulation environment but do not provide clinical guidance or real-world medical functionality.

## 1. Vital-Sign Mapping to AILEE Signals
The `VitalSignalGenerator` (`iv_logic/vital_signal_generator.hpp`) converts simulated physiological data (`ivsys::Telemetry`) into `ModelSignal` structures.
Each vital sign (e.g., Heart Rate, Simulated Blood Pressure, SpO2, Temperature) is treated as an independent model output.
* **Signal Value:** The current reading.
* **Confidence Score:** Derived from sensor quality and degraded by recent volatility (rapid, physiologically unlikely shifts).
* **Model ID:** A string identifier (e.g., `hr_monitor_v1`).

## 2. Decision Engine Validation
The `AileeDecisionEngine` (`iv_logic/ailee_decision_engine.hpp`) aggregates the `ModelSignal`s.
* It first determines a baseline action based on simple heuristics (e.g., increase flow if HR is high and BP is low).
* It then evaluates an **aggregate confidence**.
* **High Confidence (>= 0.85):** The baseline action is accepted.
* **Borderline Confidence (0.60 - 0.84):** The system applies "Grace Logic". It will allow maintaining the current flow but will reject flow changes (increase/decrease) as unsafe under uncertainty.
* **Low Confidence (< 0.60):** The system outright rejects the baseline action.

> **Threshold note:** The `CONFIDENCE_THRESHOLD_HIGH` (0.85) matches the AILEE Python
> library default (`accept_threshold`). The `CONFIDENCE_THRESHOLD_LOW` (0.60) is a
> deliberate adaptation: the AILEE Python default `grace_min` is 0.70, but the wider
> 0.60 boundary is chosen here to provide a more conservative grace window suited to
> the multi-vital-sign confidence averaging used in this simulation.

## 3. Fallback Architecture
When the `AileeDecisionEngine` rejects an action due to low or borderline confidence (when a change was proposed), it outputs a `FALLBACK_FLOW` decision and sets `AileeDecision::used_fallback = true`.
The `FlowAdjustmentPlugin` handles this by overriding the proposed rate with a safe, rolling-mean infusion rate, ensuring output stability and preventing catastrophic jumps. The rolling mean is initialised to `config::MIN_INFUSION_RATE_ML_MIN` (0.1 ml/min) to guarantee the fallback value is always within the system's configured infusion bounds.

## 4. Plugin Registration
* **`FlowAdjustmentPlugin`**: Can be used as a singleton in the simulation loop. It applies the decision to the `ivsys::ControlOutput` and generates formatted logs. Thread-safe via internal mutex.
* **`SimulationMetricsObserver`**: A thread-safe observer that tracks overall system performance, fallback frequency, stability, and latency.

## 5. Extending the System
To add new vital-sign models:
1. Update `VitalSignalGenerator::generate_signals` to process the new telemetry field.
2. Add confidence degradation rules specific to that vital.
3. The `AileeDecisionEngine` will automatically include the new signal in its aggregate confidence scoring.

## 6. REST API Exposure
The passive metrics collected by `SimulationMetricsObserver` can be retrieved via `get_metrics()`. To expose these in the REST API, the `RestApiServer` can simply call this thread-safe method and append the returned key-value pairs to its JSON status or telemetry endpoints.
