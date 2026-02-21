#pragma once

#include "iv_system_types.hpp"
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
    std::chrono::steady_clock::time_point last_check;

    const double MAX_RATE_CHANGE = 0.3;  // ml/min per cycle
    const double MIN_CARDIAC_RESERVE = 0.2;
    const double MAX_RISK_THRESHOLD = 0.75;

public:
    SafetyMonitor(const PatientProfile& prof);

    struct SafetyCheck {
        bool passed;
        double max_allowed_rate;
        std::string warnings;
    };

    SafetyCheck evaluate(double requested_rate, const PatientState& state);

    void update_volume(double rate_ml_per_min, double duration_min);

    void reset_24h_counter();

    double get_cumulative_volume() const;
};

} // namespace ivsys
