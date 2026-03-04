#include "SafetyMonitor.hpp"
#include "config_defaults.hpp"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace ivsys {

SafetyMonitor::SafetyMonitor(const PatientProfile& prof)
    : profile(prof), cumulative_volume_ml(0.0) {
    max_volume_24h_ml = profile.weight_kg * config::DAILY_VOLUME_PER_KG_ML;
    if (profile.cardiac_condition) max_volume_24h_ml *= config::CARDIAC_CONDITION_VOLUME_FACTOR;
    if (profile.renal_impairment) max_volume_24h_ml *= config::RENAL_IMPAIRMENT_VOLUME_FACTOR;
}

SafetyMonitor::SafetyCheck SafetyMonitor::evaluate(double requested_rate, const PatientState& state, double dt_minutes) {
    dt_minutes = std::max(0.0, dt_minutes);

    SafetyCheck result;
    result.passed = true;
    result.max_allowed_rate = profile.max_safe_infusion_rate;
    std::ostringstream warnings;

    // Check 1: Volume overload
    // Calculate projected volume using explicit time step
    double elapsed_h = dt_minutes / 60.0;

    double projected_volume = cumulative_volume_ml + (requested_rate * 60.0 * elapsed_h);
    if (projected_volume > max_volume_24h_ml * config::VOLUME_APPROACH_FRACTION) {
        result.max_allowed_rate = std::min(result.max_allowed_rate, config::VOLUME_LIMIT_RATE_CAP);
        warnings << "VOLUME_LIMIT_APPROACH ";
    }

    // Check 2: Cardiac load
    if (state.cardiac_reserve < MIN_CARDIAC_RESERVE) {
        result.max_allowed_rate = std::min(result.max_allowed_rate, config::LOW_CARDIAC_RATE_CAP);
        warnings << "LOW_CARDIAC_RESERVE ";
    }

    // Check 3: Rate of change limiting
    if (!recent_rates.empty()) {
        double rate_change = std::abs(requested_rate - recent_rates.back());
        if (rate_change > MAX_RATE_CHANGE) {
            double limited = recent_rates.back() +
                (requested_rate > recent_rates.back() ? MAX_RATE_CHANGE : -MAX_RATE_CHANGE);
            limited = std::max(0.0, limited);
            result.max_allowed_rate = std::min(result.max_allowed_rate, limited);
            warnings << "RATE_CHANGE_LIMITED ";
        }
    }

    // Check 4: High risk state
    if (state.risk_score > MAX_RISK_THRESHOLD) {
        result.max_allowed_rate = std::min(result.max_allowed_rate, config::HIGH_RISK_RATE_CAP);
        warnings << "HIGH_RISK_STATE ";
    }

    // Check 5: Tachycardia
    if (profile.baseline_hr_bpm > 0.0 && state.heart_rate_bpm > profile.baseline_hr_bpm * config::TACHYCARDIA_HR_MULTIPLIER) {
        result.max_allowed_rate = std::min(result.max_allowed_rate, config::TACHYCARDIA_RATE_CAP);
        warnings << "TACHYCARDIA_DETECTED ";
    }

    // Check 6: Minimum safe rate
    if (result.max_allowed_rate < config::EMERGENCY_MIN_RATE && state.hydration_pct < config::EMERGENCY_HYDRATION_THRESHOLD) {
        result.max_allowed_rate = config::EMERGENCY_MIN_RATE;
        warnings << "EMERGENCY_MIN_RATE ";
    }

    result.warnings = warnings.str();
    result.passed = (result.max_allowed_rate >= config::EMERGENCY_MIN_RATE);

    return result;
}

void SafetyMonitor::update_volume(double rate_ml_per_min, double duration_min) {
    rate_ml_per_min = std::max(0.0, rate_ml_per_min);
    duration_min = std::max(0.0, duration_min);
    cumulative_volume_ml += rate_ml_per_min * duration_min;
    recent_rates.push_back(rate_ml_per_min);
    if (recent_rates.size() > 20) recent_rates.pop_front();
}

void SafetyMonitor::reset_24h_counter() {
    cumulative_volume_ml = 0.0;
}

double SafetyMonitor::get_cumulative_volume() const { return cumulative_volume_ml; }

} // namespace ivsys
