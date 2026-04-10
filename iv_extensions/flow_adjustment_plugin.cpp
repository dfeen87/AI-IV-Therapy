#include "flow_adjustment_plugin.hpp"
#include <sstream>
#include <chrono>

namespace ivsys {
namespace extensions {

FlowAdjustmentPlugin::FlowAdjustmentPlugin() {}

FlowAdjustmentPlugin& FlowAdjustmentPlugin::get_instance() {
    static FlowAdjustmentPlugin instance;
    return instance;
}

void FlowAdjustmentPlugin::update_rolling_mean(double new_rate) {
    // Simple EWMA for rolling mean
    rolling_mean_infusion_ = (0.9 * rolling_mean_infusion_) + (0.1 * new_rate);
}

void FlowAdjustmentPlugin::apply_decision(const AileeDecision& decision, ivsys::ControlOutput& current_control) {
    std::ostringstream log_stream;
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    log_stream << "[" << ms << "] FLOW_PLUGIN: ";

    double current_rate = current_control.infusion_ml_per_min;
    double new_rate = current_rate;
    bool fallback_active = false;

    switch (decision.action) {
        case DecisionAction::INCREASE_FLOW:
            new_rate = current_rate + 5.0; // 5ml/min step
            log_stream << "Action=INCREASE ";
            break;
        case DecisionAction::DECREASE_FLOW:
            new_rate = current_rate - 5.0;
            if (new_rate < 0.0) new_rate = 0.0;
            log_stream << "Action=DECREASE ";
            break;
        case DecisionAction::MAINTAIN_FLOW:
            log_stream << "Action=MAINTAIN ";
            break;
        case DecisionAction::FALLBACK_FLOW:
            new_rate = rolling_mean_infusion_;
            fallback_active = true;
            log_stream << "Action=FALLBACK (Rate set to rolling mean: " << new_rate << ") ";
            current_control.safety_override = true;
            break;
    }

    current_control.infusion_ml_per_min = new_rate;
    current_control.rationale = decision.reasoning;
    current_control.confidence = decision.aggregate_confidence;

    if (!fallback_active) {
        update_rolling_mean(new_rate);
    }

    log_stream << "| Conf=" << decision.aggregate_confidence
               << " | FallbackActive=" << (fallback_active ? "TRUE" : "FALSE")
               << " | Sigs=" << decision.contributing_signals.size();

    latest_log_ = log_stream.str();
}

} // namespace extensions
} // namespace ivsys