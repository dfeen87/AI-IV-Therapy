#include "flow_adjustment_plugin.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <ctime>

namespace ivsys {
namespace extensions {

FlowAdjustmentPlugin::FlowAdjustmentPlugin() {}

FlowAdjustmentPlugin& FlowAdjustmentPlugin::get_instance() {
    static FlowAdjustmentPlugin instance;
    return instance;
}

std::string FlowAdjustmentPlugin::get_latest_log() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return latest_log_;
}

void FlowAdjustmentPlugin::update_rolling_mean(double new_rate) {
    // Simple EWMA for rolling mean
    rolling_mean_infusion_ = (0.9 * rolling_mean_infusion_) + (0.1 * new_rate);
}

void FlowAdjustmentPlugin::apply_decision(const AileeDecision& decision, ivsys::ControlOutput& current_control) {
    // Build ISO 8601 timestamp consistent with SystemLogger/RestApiServer format.
    // gmtime_r is used (POSIX) for reentrant, thread-safe time conversion.
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::tm tm_buf{};
    gmtime_r(&time_t_now, &tm_buf);
    std::ostringstream ts_stream;
    ts_stream << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
    ts_stream << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';

    std::ostringstream log_stream;
    log_stream << "[" << ts_stream.str() << "] FLOW_PLUGIN: ";

    double current_rate = current_control.infusion_ml_per_min;
    double new_rate = current_rate;
    bool fallback_active = false;

    switch (decision.action) {
        case DecisionAction::INCREASE_FLOW:
            // Step size matches MAX_RATE_CHANGE_ML_MIN; result is clamped to system bounds.
            new_rate = current_rate + ivsys::config::MAX_RATE_CHANGE_ML_MIN;
            new_rate = std::min(new_rate, ivsys::config::MAX_INFUSION_RATE_ML_MIN);
            log_stream << "Action=INCREASE ";
            break;
        case DecisionAction::DECREASE_FLOW:
            new_rate = current_rate - ivsys::config::MAX_RATE_CHANGE_ML_MIN;
            new_rate = std::max(new_rate, ivsys::config::MIN_INFUSION_RATE_ML_MIN);
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
            current_control.warning_flags = "AILEE_FALLBACK: " + decision.reasoning;
            break;
    }

    std::lock_guard<std::mutex> lock(mutex_);

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