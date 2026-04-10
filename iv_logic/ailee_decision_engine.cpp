#include "ailee_decision_engine.hpp"
#include <numeric>

namespace ivsys {
namespace extensions {

AileeDecisionEngine::AileeDecisionEngine() {}

double AileeDecisionEngine::calculate_aggregate_confidence(const std::vector<ModelSignal>& signals) const {
    if (signals.empty()) return 0.0;

    double sum = 0.0;
    for (const auto& sig : signals) {
        sum += sig.confidence;
    }
    return sum / signals.size();
}

DecisionAction AileeDecisionEngine::determine_baseline_action(const std::vector<ModelSignal>& signals) const {
    // Simulation heuristic: if HR is elevated and BP is low, increase flow;
    // if BP is elevated, decrease flow. Thresholds are simulation-only and
    // are not clinical diagnostic criteria.
    double hr = 0.0;
    double bp = 0.0;

    for (const auto& sig : signals) {
        if (sig.model_id == "hr_monitor_v1")   hr = sig.value;
        if (sig.model_id == "nibp_monitor_v1") bp = sig.value;
    }

    if (hr > HEURISTIC_HR_TACHYCARDIA_BPM && bp < HEURISTIC_BP_HYPOTENSION_MMHG)
        return DecisionAction::INCREASE_FLOW;
    if (bp > HEURISTIC_BP_HYPERTENSION_MMHG)
        return DecisionAction::DECREASE_FLOW;

    return DecisionAction::MAINTAIN_FLOW;
}

AileeDecision AileeDecisionEngine::process_signals(const std::vector<ModelSignal>& signals) {
    AileeDecision decision;

    decision.aggregate_confidence = calculate_aggregate_confidence(signals);
    decision.used_fallback = false;

    for (const auto& sig : signals) {
        decision.contributing_signals.push_back(sig.model_id + " (conf: " + std::to_string(sig.confidence) + ")");
    }

    DecisionAction proposed_action = determine_baseline_action(signals);

    // AILEE Logic Application
    if (decision.aggregate_confidence >= CONFIDENCE_THRESHOLD_HIGH) {
        // High confidence -> accept proposed action
        decision.action = proposed_action;
        decision.reasoning = "[AILEE: ACCEPTED] High aggregate confidence. Consensus reached.";
    } else if (decision.aggregate_confidence >= CONFIDENCE_THRESHOLD_LOW) {
        // Borderline confidence -> Grace Logic
        if (proposed_action == DecisionAction::MAINTAIN_FLOW) {
            decision.action = DecisionAction::MAINTAIN_FLOW;
            decision.reasoning = "[AILEE: GRACE] Borderline confidence. Mediating to MAINTAIN (safe).";
        } else {
            // Unsafe to change flow with borderline confidence
            decision.action = DecisionAction::FALLBACK_FLOW;
            decision.used_fallback = true;
            decision.reasoning = "[AILEE: GRACE FAILED] Borderline confidence. Unsafe to apply " +
                                 std::string(proposed_action == DecisionAction::INCREASE_FLOW ? "INCREASE" : "DECREASE") +
                                 ". Triggering FALLBACK.";
        }
    } else {
        // Low confidence -> Outright Rejected -> Fallback
        decision.action = DecisionAction::FALLBACK_FLOW;
        decision.used_fallback = true;
        decision.reasoning = "[AILEE: REJECTED] Aggregate confidence below safe threshold. Triggering FALLBACK.";
    }

    return decision;
}

} // namespace extensions
} // namespace ivsys