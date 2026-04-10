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
};

class AileeDecisionEngine {
public:
    AileeDecisionEngine();

    // Process vital signals and return a validated decision based on AILEE principles.
    AileeDecision process_signals(const std::vector<ModelSignal>& signals);

private:
    double calculate_aggregate_confidence(const std::vector<ModelSignal>& signals) const;
    DecisionAction determine_baseline_action(const std::vector<ModelSignal>& signals) const;

    // AILEE Trust Thresholds
    const double CONFIDENCE_THRESHOLD_HIGH = 0.85;
    const double CONFIDENCE_THRESHOLD_LOW = 0.60;
};

} // namespace extensions
} // namespace ivsys
