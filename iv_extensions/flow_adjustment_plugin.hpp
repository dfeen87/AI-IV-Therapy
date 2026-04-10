#pragma once

#include "../iv_logic/ailee_decision_engine.hpp"
#include "../src/iv_system_types.hpp"
#include "../src/config_defaults.hpp"
#include <string>
#include <mutex>

namespace ivsys {
namespace extensions {

// Not thread-safe when using the instance directly; singleton access via
// get_instance() is protected internally per method with mutex_.
class FlowAdjustmentPlugin {
public:
    FlowAdjustmentPlugin();

    // Self-registering style, though explicitly called in our integration
    static FlowAdjustmentPlugin& get_instance();

    // Applies the validated decision to the IV simulation's ControlOutput.
    // Thread-safe: protected by internal mutex.
    void apply_decision(const AileeDecision& decision, ivsys::ControlOutput& current_control);

    // Get latest log string for the observer.
    // Thread-safe: protected by internal mutex.
    std::string get_latest_log() const;

private:
    mutable std::mutex mutex_;
    std::string latest_log_;

    // Fallback logic: EWMA of accepted infusion rates.
    // Initialised to the system minimum so that a pre-warmup fallback is safe.
    double rolling_mean_infusion_ = ivsys::config::MIN_INFUSION_RATE_ML_MIN;
    void update_rolling_mean(double new_rate);
};

} // namespace extensions
} // namespace ivsys
