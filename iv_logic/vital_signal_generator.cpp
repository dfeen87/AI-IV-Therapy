#include "vital_signal_generator.hpp"
#include <cmath>
#include <algorithm>

namespace ivsys {
namespace extensions {

VitalSignalGenerator::VitalSignalGenerator() {}

double VitalSignalGenerator::calculate_confidence(double base_confidence, double stability_penalty, std::string& reason) {
    double confidence = base_confidence - stability_penalty;

    if (confidence < 0.0) confidence = 0.0;
    if (confidence > 1.0) confidence = 1.0;

    if (stability_penalty > 0.2) {
        reason = "High Volatility";
    } else if (base_confidence < 0.5) {
        reason = "Poor Sensor Quality";
    } else {
        reason = "Stable";
    }

    return confidence;
}

std::vector<ModelSignal> VitalSignalGenerator::generate_signals(const ivsys::Telemetry& telemetry) {
    std::vector<ModelSignal> signals;

    double hr = telemetry.heart_rate_bpm;
    double spo2 = telemetry.spo2_pct;
    double temp = telemetry.temp_celsius;
    // Derive a simulated BP (just for modeling purposes in the extension)
    // A very simplistic simulation of BP based on HR and hydration
    double bp_sys = 120.0 + (hr - 70.0) * 0.5 - (100.0 - telemetry.hydration_pct) * 0.2;

    double sensor_quality = telemetry.signal_quality;
    if (sensor_quality == 0.0) {
        sensor_quality = 0.8; // default if unset
    }

    if (first_run_) {
        prev_hr_ = hr;
        prev_bp_sys_ = bp_sys;
        prev_spo2_ = spo2;
        prev_temp_ = temp;
        first_run_ = false;
    }

    // 1. Heart Rate
    {
        double delta_hr = std::abs(hr - prev_hr_);
        double stability_penalty = std::min(delta_hr / 20.0, 0.5); // Penalty if HR jumps > 20 bpm
        std::string reason;
        double conf = calculate_confidence(sensor_quality, stability_penalty, reason);
        signals.push_back({hr, conf, "hr_monitor_v1", reason});
        prev_hr_ = hr;
    }

    // 2. Blood Pressure (Simulated)
    {
        double delta_bp = std::abs(bp_sys - prev_bp_sys_);
        double stability_penalty = std::min(delta_bp / 30.0, 0.5);
        std::string reason;
        double conf = calculate_confidence(sensor_quality * 0.9, stability_penalty, reason); // BP slightly less confident
        signals.push_back({bp_sys, conf, "nibp_monitor_v1", reason});
        prev_bp_sys_ = bp_sys;
    }

    // 3. Oxygen Saturation
    {
        double delta_spo2 = std::abs(spo2 - prev_spo2_);
        double stability_penalty = std::min(delta_spo2 / 5.0, 0.5);
        std::string reason;
        double conf = calculate_confidence(sensor_quality, stability_penalty, reason);
        signals.push_back({spo2, conf, "pulse_ox_v1", reason});
        prev_spo2_ = spo2;
    }

    // 4. Temperature
    {
        double delta_temp = std::abs(temp - prev_temp_);
        double stability_penalty = std::min(delta_temp / 1.0, 0.5);
        std::string reason;
        double conf = calculate_confidence(sensor_quality, stability_penalty, reason);
        signals.push_back({temp, conf, "temp_probe_v1", reason});
        prev_temp_ = temp;
    }

    // 5. Respiratory Rate (Simulated based on HR & Lactate)
    {
        double resp_rate = 16.0 + (hr - 70.0) * 0.1 + telemetry.lactate_mmol * 1.5;
        signals.push_back({resp_rate, sensor_quality * 0.85, "resp_estimator_v1", "Derived Stable"});
    }

    return signals;
}

} // namespace extensions
} // namespace ivsys