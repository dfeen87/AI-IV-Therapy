#include "SafetyMonitor.hpp"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace ivsys {

SafetyMonitor::SafetyMonitor(const PatientProfile& prof)
    : profile(prof), cumulative_volume_ml(0.0), last_check(std::chrono::steady_clock::now()) {
    max_volume_24h_ml = profile.weight_kg * 35.0;
    if (profile.cardiac_condition) max_volume_24h_ml *= 0.7;
    if (profile.renal_impairment) max_volume_24h_ml *= 0.6;
}

SafetyMonitor::SafetyCheck SafetyMonitor::evaluate(double requested_rate, const PatientState& state) {
    SafetyCheck result;
    result.passed = true;
    result.max_allowed_rate = profile.max_safe_infusion_rate;
    std::ostringstream warnings;

    // Check 1: Volume overload
    auto now = std::chrono::steady_clock::now();
    auto elapsed_h = std::chrono::duration<double, std::ratio<3600>>(
        now - last_check).count();
    last_check = now;

    double projected_volume = cumulative_volume_ml + (requested_rate * 60.0 * elapsed_h);
    if (projected_volume > max_volume_24h_ml * 0.9) {
        result.max_allowed_rate = std::min(result.max_allowed_rate, 0.3);
        warnings << "VOLUME_LIMIT_APPROACH ";
    }

    // Check 2: Cardiac load
    if (state.cardiac_reserve < MIN_CARDIAC_RESERVE) {
        result.max_allowed_rate = std::min(result.max_allowed_rate, 0.5);
        warnings << "LOW_CARDIAC_RESERVE ";
    }

    // Check 3: Rate of change limiting
    if (!recent_rates.empty()) {
        double rate_change = std::abs(requested_rate - recent_rates.back());
        if (rate_change > MAX_RATE_CHANGE) {
            double limited = recent_rates.back() +
                (requested_rate > recent_rates.back() ? MAX_RATE_CHANGE : -MAX_RATE_CHANGE);
            result.max_allowed_rate = std::min(result.max_allowed_rate, limited);
            warnings << "RATE_CHANGE_LIMITED ";
        }
    }

    // Check 4: High risk state
    if (state.risk_score > MAX_RISK_THRESHOLD) {
        result.max_allowed_rate = std::min(result.max_allowed_rate, 0.6);
        warnings << "HIGH_RISK_STATE ";
    }

    // Check 5: Tachycardia
    if (state.heart_rate_bpm > profile.baseline_hr_bpm * 1.4) {
        result.max_allowed_rate = std::min(result.max_allowed_rate, 0.4);
        warnings << "TACHYCARDIA_DETECTED ";
    }

    // Check 6: Minimum safe rate
    if (result.max_allowed_rate < 0.1 && state.hydration_pct < 50.0) {
        result.max_allowed_rate = 0.1;
        warnings << "EMERGENCY_MIN_RATE ";
    }

    result.warnings = warnings.str();
    result.passed = (result.max_allowed_rate >= 0.1);

    return result;
}

void SafetyMonitor::update_volume(double rate_ml_per_min, double duration_min) {
    cumulative_volume_ml += rate_ml_per_min * duration_min;
    recent_rates.push_back(rate_ml_per_min);
    if (recent_rates.size() > 20) recent_rates.pop_front();
}

void SafetyMonitor::reset_24h_counter() {
    cumulative_volume_ml = 0.0;
}

double SafetyMonitor::get_cumulative_volume() const { return cumulative_volume_ml; }

} // namespace ivsys
