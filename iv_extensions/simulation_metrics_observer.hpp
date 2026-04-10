#pragma once

#include <mutex>
#include <atomic>
#include <map>
#include <string>
#include "../iv_logic/ailee_decision_engine.hpp"

namespace ivsys {
namespace extensions {

class SimulationMetricsObserver {
public:
    SimulationMetricsObserver();

    // Singleton access
    static SimulationMetricsObserver& get_instance();

    // Called passively when a new decision is made
    void observe_decision(const AileeDecision& decision, double latency_ms);
    void observe_signals(const std::vector<ModelSignal>& signals);

    // Read-only access for REST API compatibility
    std::map<std::string, std::string> get_metrics() const;

private:
    mutable std::mutex mutex_;

    // Metrics
    unsigned long total_adjustments_;
    unsigned long fallback_count_;
    double cumulative_confidence_;
    unsigned long decision_count_;
    double cumulative_latency_ms_;

    // Volatility tracking
    double prev_aggregate_confidence_;
    double stability_score_; // 0-100

    // Vital-signal tracking
    std::map<std::string, double> prev_signal_values_;
    double cumulative_signal_volatility_;
};

} // namespace extensions
} // namespace ivsys