#include "simulation_metrics_observer.hpp"
#include <cmath>

namespace ivsys {
namespace extensions {

SimulationMetricsObserver::SimulationMetricsObserver()
    : total_adjustments_(0), fallback_count_(0), cumulative_confidence_(0.0),
      decision_count_(0), cumulative_latency_ms_(0.0),
      prev_aggregate_confidence_(1.0), stability_score_(100.0),
      cumulative_signal_volatility_(0.0) {}

SimulationMetricsObserver& SimulationMetricsObserver::get_instance() {
    static SimulationMetricsObserver instance;
    return instance;
}

void SimulationMetricsObserver::observe_decision(const AileeDecision& decision, double latency_ms) {
    std::lock_guard<std::mutex> lock(mutex_);

    decision_count_++;
    cumulative_latency_ms_ += latency_ms;
    cumulative_confidence_ += decision.aggregate_confidence;

    if (decision.action == DecisionAction::INCREASE_FLOW || decision.action == DecisionAction::DECREASE_FLOW) {
        total_adjustments_++;
    } else if (decision.action == DecisionAction::FALLBACK_FLOW) {
        fallback_count_++;
    }

    // Update stability score based on confidence volatility
    double conf_change = std::abs(decision.aggregate_confidence - prev_aggregate_confidence_);
    stability_score_ = (stability_score_ * 0.95) + ((1.0 - conf_change) * 100.0 * 0.05);
    prev_aggregate_confidence_ = decision.aggregate_confidence;
}

void SimulationMetricsObserver::observe_signals(const std::vector<ModelSignal>& signals) {
    std::lock_guard<std::mutex> lock(mutex_);

    double current_volatility = 0.0;
    for (const auto& sig : signals) {
        if (prev_signal_values_.find(sig.model_id) != prev_signal_values_.end()) {
            current_volatility += std::abs(sig.value - prev_signal_values_[sig.model_id]);
        }
        prev_signal_values_[sig.model_id] = sig.value;
    }

    if (!signals.empty()) {
        cumulative_signal_volatility_ += (current_volatility / signals.size());
    }
}

std::map<std::string, std::string> SimulationMetricsObserver::get_metrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, std::string> metrics;

    metrics["total_adjustments"] = std::to_string(total_adjustments_);
    metrics["fallback_frequency"] = decision_count_ > 0 ? std::to_string((double)fallback_count_ / decision_count_) : "0.0";
    metrics["average_confidence"] = decision_count_ > 0 ? std::to_string(cumulative_confidence_ / decision_count_) : "0.0";
    metrics["stability_score"] = std::to_string(stability_score_);
    metrics["average_decision_latency_ms"] = decision_count_ > 0 ? std::to_string(cumulative_latency_ms_ / decision_count_) : "0.0";
    metrics["vital_signal_volatility"] = decision_count_ > 0 ? std::to_string(cumulative_signal_volatility_ / decision_count_) : "0.0";

    return metrics;
}

} // namespace extensions
} // namespace ivsys