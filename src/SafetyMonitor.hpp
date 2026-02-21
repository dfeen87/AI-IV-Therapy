#pragma once

#include "iv_system_types.hpp"
#include "config_defaults.hpp"
#include <deque>
#include <chrono>
#include <string>

namespace ivsys {

class SafetyMonitor {
private:
    PatientProfile profile;
    double cumulative_volume_ml;
    double max_volume_24h_ml;
    std::deque<double> recent_rates;
    // Removed internal time state 'last_check' to make evaluate pure/stateless regarding time

    const double MAX_RATE_CHANGE    = config::MAX_RATE_CHANGE_ML_MIN;
    const double MIN_CARDIAC_RESERVE = config::MIN_CARDIAC_RESERVE;
    const double MAX_RISK_THRESHOLD  = config::HIGH_RISK_THRESHOLD;

public:
    SafetyMonitor(const PatientProfile& prof);

    struct SafetyCheck {
        bool passed;
        double max_allowed_rate;
        std::string warnings;
    };

    // Updated to accept explicit time delta (dt_minutes)
    SafetyCheck evaluate(double requested_rate, const PatientState& state, double dt_minutes);

    void update_volume(double rate_ml_per_min, double duration_min);

    void reset_24h_counter();

    double get_cumulative_volume() const;
};

} // namespace ivsys
