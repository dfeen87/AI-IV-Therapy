#pragma once

#include <string>
#include <cstdint>

namespace ivsys {

// -----------------------------
// Patient Profile
// -----------------------------
struct PatientProfile {
    double weight_kg;
    uint32_t age_years;
    bool cardiac_condition;
    double baseline_hr_bpm;
    double max_safe_infusion_rate_ml_min;
};

// -----------------------------
// Telemetry Snapshot
// -----------------------------
struct Telemetry {
    double hydration_pct;
    double heart_rate_bpm;
    double spo2_pct;
    double temperature_c;
    double lactate_mmol;
    double fatigue_idx;
    double blood_loss_idx;
};

// -----------------------------
// Derived State
// -----------------------------
struct SystemState {
    double energy_proxy;
    double cardiac_reserve;
    double metabolic_load;
    double composite_risk;
    double signal_coherence;
};

// -----------------------------
// Control Output
// -----------------------------
struct ControlCommand {
    double desired_rate_ml_min;
    double safety_limited_rate_ml_min;
    std::string rationale;
};

// -----------------------------
// Safety Limits
// -----------------------------
struct SafetyLimits {
    double min_rate_ml_min = 0.1;
    double max_rate_ml_min = 1.5;
    double max_daily_volume_ml = 3000.0;
};

} // namespace ivsys
