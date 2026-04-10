#pragma once

#include "../iv_logic/ailee_decision_engine.hpp"
#include "../src/iv_system_types.hpp"
#include <string>

namespace ivsys {
namespace extensions {

class FlowAdjustmentPlugin {
public:
    FlowAdjustmentPlugin();

    // Self-registering style, though explicitly called in our integration
    static FlowAdjustmentPlugin& get_instance();

    // Applies the validated decision to the IV simulation's ControlOutput
    void apply_decision(const AileeDecision& decision, ivsys::ControlOutput& current_control);

    // Get latest log string for the observer
    std::string get_latest_log() const { return latest_log_; }

private:
    std::string latest_log_;

    // Fallback logic
    double rolling_mean_infusion_ = 50.0; // Default startup
    void update_rolling_mean(double new_rate);
};

} // namespace extensions
} // namespace ivsys
