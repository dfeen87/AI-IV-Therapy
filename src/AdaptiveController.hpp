#pragma once

#include "iv_system_types.hpp"
#include "SafetyMonitor.hpp"
#include "StateEstimator.hpp"
#include <string>

namespace ivsys {

class AdaptiveController {
private:
    PatientProfile profile;
    double last_command;

    double calculate_base_rate(const PatientState& state);
    double apply_coherence_modulation(double base_rate, const PatientState& state);
    double apply_cardiac_limiting(double rate, const PatientState& state);
    std::string generate_rationale(const PatientState& state, double rate,
                                   bool safety_limited, bool predictive_boost);

public:
    AdaptiveController(const PatientProfile& prof);

    ControlOutput decide(const PatientState& state, SafetyMonitor& safety,
                        StateEstimator& estimator);
};

} // namespace ivsys
