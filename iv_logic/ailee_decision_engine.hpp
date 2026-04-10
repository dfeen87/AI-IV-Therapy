#pragma once

#include <vector>
#include <string>
#include "vital_signal_generator.hpp"

namespace ivsys {
namespace extensions {

enum class DecisionAction {
    INCREASE_FLOW,
    DECREASE_FLOW,
    MAINTAIN_FLOW,
    FALLBACK_FLOW
};

struct AileeDecision {
    DecisionAction action;
    std::string reasoning;
    std::vector<std::string> contributing_signals;
    double aggregate_confidence;
    bool used_fallback = false; // true when FALLBACK_FLOW is triggered
};

class AileeDecisionEngine {
public:
    AileeDecisionEngine();

    // Process vital signals and return a validated decision based on AILEE principles.
    AileeDecision process_signals(const std::vector<ModelSignal>& signals);

private:
    double calculate_aggregate_confidence(const std::vector<ModelSignal>& signals) const;
    DecisionAction determine_baseline_action(const std::vector<ModelSignal>& signals) const;

    // AILEE Trust Thresholds.
    // CONFIDENCE_THRESHOLD_HIGH matches the AILEE Python default (accept_threshold = 0.85).
    // CONFIDENCE_THRESHOLD_LOW is intentionally set to 0.60 rather than the AILEE Python
    // default grace_min of 0.70, providing a wider grace window suited to the simulation's
    // multi-vital-sign confidence averaging.
    static constexpr double CONFIDENCE_THRESHOLD_HIGH = 0.85;
    static constexpr double CONFIDENCE_THRESHOLD_LOW  = 0.60;

    // Simulation heuristic thresholds for baseline action determination.
    // These are not clinical diagnostic criteria; they are conservative triggers
    // chosen to exercise the decision engine across a realistic simulation range.
    static constexpr double HEURISTIC_HR_TACHYCARDIA_BPM  = 110.0;
    static constexpr double HEURISTIC_BP_HYPOTENSION_MMHG = 90.0;
    static constexpr double HEURISTIC_BP_HYPERTENSION_MMHG = 140.0;
};

} // namespace extensions
} // namespace ivsys
