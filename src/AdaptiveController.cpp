#include "AdaptiveController.hpp"
#include "Utils.hpp"
#include <sstream>
#include <iomanip>

namespace ivsys {

double AdaptiveController::calculate_base_rate(const PatientState& state) {
    double hydration_deficit = (100.0 - state.hydration_pct) / 100.0;
    double hydration_urgency = hydration_deficit < 0.5 ?
        hydration_deficit : Utils::sigmoid(hydration_deficit, 0.5, 5.0);

    double energy_gap = 1.0 - state.energy_T;
    double energy_need = energy_gap * (1.0 + 0.5 * state.metabolic_load);

    double risk_amplifier = 1.0 + 0.5 * state.risk_score;

    double base = 0.4 + 1.4 * Utils::clamp(
        (0.6 * hydration_urgency + 0.4 * energy_need) * risk_amplifier,
        0.0, 1.0
    );

    return base;
}

double AdaptiveController::apply_coherence_modulation(double base_rate, const PatientState& state) {
    return base_rate * state.coherence_sigma;
}

double AdaptiveController::apply_cardiac_limiting(double rate, const PatientState& state) {
    if (state.cardiac_reserve < 0.3) {
        double scaling = 0.5 + 0.5 * Utils::sigmoid(state.cardiac_reserve, 0.3, 10.0);
        return rate * scaling;
    }
    return rate;
}

std::string AdaptiveController::generate_rationale(const PatientState& state, double rate,
                               bool safety_limited, bool predictive_boost) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "H=" << state.hydration_pct << "% ";
    oss << "E_T=" << state.energy_T << " ";
    oss << "T=" << state.energy_T_absolute << "W/kg ";
    oss << "R=" << state.risk_score << " ";
    oss << "C_res=" << state.cardiac_reserve << " ";
    oss << "σ=" << state.coherence_sigma << " ";
    oss << "v=" << state.estimated_flow_velocity_cm_s << "cm/s ";
    oss << "G(v)=" << state.flow_efficiency << " ";
    oss << "u=" << rate << "ml/min";
    if (safety_limited) oss << " [SAFETY_LIM]";
    if (predictive_boost) oss << " [PRED_BOOST]";
    return oss.str();
}

AdaptiveController::AdaptiveController(const PatientProfile& prof)
    : profile(prof), last_command(0.4) {}

ControlOutput AdaptiveController::decide(const PatientState& state, SafetyMonitor& safety,
                    StateEstimator& estimator) {
    ControlOutput output;
    bool predictive_boost = false;

    // Step 1: Calculate desired rate
    double desired_rate = calculate_base_rate(state);

    // Step 2: Predictive control
    auto predicted = estimator.predict_forward(10);
    if (predicted.has_value() && predicted->hydration_pct < 50.0) {
        desired_rate *= 1.2;
        predictive_boost = true;
    }

    // Step 3: Apply modulation
    desired_rate = apply_coherence_modulation(desired_rate, state);
    desired_rate = apply_cardiac_limiting(desired_rate, state);

    // Step 4: Clamp to physiological bounds
    desired_rate = Utils::clamp(desired_rate, 0.1, profile.max_safe_infusion_rate);

    // Step 5: Safety evaluation
    auto safety_check = safety.evaluate(desired_rate, state);

    bool safety_limited = false;
    if (desired_rate > safety_check.max_allowed_rate) {
        desired_rate = safety_check.max_allowed_rate;
        safety_limited = true;
    }

    // Step 6: Final output
    output.infusion_ml_per_min = desired_rate;
    output.confidence = 1.0 - state.uncertainty;
    output.rationale = generate_rationale(state, desired_rate, safety_limited, predictive_boost);
    output.safety_override = !safety_check.passed;
    output.warning_flags = safety_check.warnings;

    last_command = desired_rate;
    return output;
}

} // namespace ivsys
