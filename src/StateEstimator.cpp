#include "StateEstimator.hpp"
#include "Utils.hpp"
#include <cmath>
#include <algorithm>

#ifdef ENABLE_NEURAL_ESTIMATOR
#include "NeuralStateEstimator.hpp"
#include <iostream>
static ivsys::NeuralStateEstimator g_neural_estimator;
static bool g_neural_init = false;
static void init_neural_estimator() {
    if (g_neural_init) return;
    g_neural_init = true;
    try {
        g_neural_estimator.load(NEURAL_MODEL_PATH);
    } catch (const std::exception& e) {
        std::cerr << "[NeuralEstimator] WARNING: could not load model ("
                  << e.what() << "); falling back to rule-based estimator.\n";
    }
}
#endif

namespace ivsys {

double StateEstimator::calculate_coherence(const Telemetry& m) {
    double base_coherence = m.signal_quality;

    if (m.heart_rate_bpm < 40 || m.heart_rate_bpm > 180) base_coherence *= 0.5;
    if (m.temp_celsius < 35.0 || m.temp_celsius > 40.0) base_coherence *= 0.7;
    if (m.spo2_pct < 85.0) base_coherence *= 0.6;

    if (telemetry_history.size() >= 5) {
        double hr_variance = 0.0;
        size_t start_idx = telemetry_history.size() - 5;
        for (size_t i = start_idx; i < telemetry_history.size(); ++i) {
            hr_variance += std::pow(telemetry_history[i].heart_rate_bpm - m.heart_rate_bpm, 2);
        }
        hr_variance /= 5.0;

        if (hr_variance > 400.0) base_coherence *= 0.7;
    }

    return Utils::clamp(base_coherence, 0.1, 1.0);
}

double StateEstimator::estimate_flow_velocity(const Telemetry& m, double infusion_rate_ml_min,
                               double weight_kg) {
    double cardiac_flow_ml_s = m.cardiac_output_L_min * 1000.0 / 60.0;
    double infusion_flow_ml_s = infusion_rate_ml_min / 60.0;
    double total_flow_ml_s = cardiac_flow_ml_s + infusion_flow_ml_s;

    double effective_area_cm2 = std::max(1.0, weight_kg * 0.5);
    double v_estimated = total_flow_ml_s / effective_area_cm2;
    return Utils::clamp(v_estimated, 0.05, 40.0);
}

double StateEstimator::calculate_tissue_efficiency(const Telemetry& m,
                                   const EnergyTransferParams& params,
                                   double perfusion_state) {
    double base_efficiency = params.eta_muscle;

    if (m.spo2_pct < 90.0) {
        double hypoxia_penalty = (90.0 - m.spo2_pct) / 20.0;
        base_efficiency *= (1.0 - Utils::clamp(hypoxia_penalty, 0.0, 0.6));
    }

    base_efficiency *= (0.5 + 0.5 * perfusion_state);

    if (m.temp_celsius < 36.0) {
        double temp_penalty = (36.0 - m.temp_celsius) / 5.0;
        base_efficiency *= (1.0 - Utils::clamp(temp_penalty, 0.0, 0.4));
    }

    return Utils::clamp(base_efficiency, params.eta_ischemic, params.eta_brain_heart);
}

double StateEstimator::calculate_energy_transfer_absolute(const Telemetry& m,
                                           const EnergyTransferParams& params,
                                           double infusion_rate_ml_min,
                                           double weight_kg,
                                           double perfusion_state) {
    double P_input = params.P_baseline + params.P_iv_supplement + params.P_energy_cells;

    double v = estimate_flow_velocity(m, infusion_rate_ml_min, weight_kg);
    double G_v = Utils::gaussian(v, params.v_optimal_cm_s, params.sigma_velocity);

    double m_dot = infusion_rate_ml_min / 60000.0;

    double I_sp = params.I_sp_standard * 1000.0;

    double eta = calculate_tissue_efficiency(m, params, perfusion_state);

    double infusion_power = m_dot * I_sp * eta * G_v;
    double T_t = (P_input + infusion_power) / weight_kg;

    return T_t;
}

double StateEstimator::calculate_energy_proxy(const Telemetry& m) {
    double h_term = Utils::sigmoid(m.hydration_pct, 60.0, 0.1);

    double b_term = Utils::exponential_decay(m.blood_loss_idx, 3.0);

    double f_term;
    if (m.fatigue_idx < 0.7) {
        f_term = 1.0 - m.fatigue_idx;
    } else {
        f_term = 0.3 * (1.0 - m.fatigue_idx);
    }

    double o_term = Utils::sigmoid(m.spo2_pct, 92.0, 0.3);

    double l_term = Utils::exponential_decay(std::max(0.0, m.lactate_mmol - 2.0), 0.5);

    double energy = 0.30 * h_term + 0.25 * b_term + 0.20 * f_term +
                   0.15 * o_term + 0.10 * l_term;

    return Utils::clamp(energy, 0.0, 1.0);
}

double StateEstimator::calculate_metabolic_load(const Telemetry& m) {
    double hr_stress = Utils::clamp((m.heart_rate_bpm - 60.0) / 100.0, 0.0, 1.0);
    double temp_stress = std::abs(m.temp_celsius - 37.0) / 3.0;
    double lactate_stress = Utils::clamp(m.lactate_mmol / 10.0, 0.0, 1.0);
    double anxiety_stress = m.anxiety_idx;

    return Utils::clamp(0.3*hr_stress + 0.25*temp_stress +
                       0.25*lactate_stress + 0.2*anxiety_stress, 0.0, 1.0);
}

double StateEstimator::calculate_cardiac_reserve(const Telemetry& m, double age_years) {
    double max_predicted_hr = 220.0 - age_years;  // Fox formula (1971) for age-predicted HRmax
    double current_percentage = m.heart_rate_bpm / max_predicted_hr;

    double reserve = 1.0 - Utils::sigmoid(current_percentage, 0.85, 10.0);

    reserve *= Utils::clamp(m.spo2_pct / 95.0, 0.5, 1.0);

    return Utils::clamp(reserve, 0.0, 1.0);
}

double StateEstimator::calculate_risk_score(const Telemetry& m, double energy_T) {
    double blood_loss_risk = m.blood_loss_idx;
    double hypoxia_risk = Utils::clamp((95.0 - m.spo2_pct) / 10.0, 0.0, 1.0);
    double hypothermia_risk = std::max(0.0, (36.0 - m.temp_celsius) / 2.0);

    double R_critical = std::max({blood_loss_risk, hypoxia_risk, hypothermia_risk});

    double dehydration_risk = Utils::clamp((100.0 - m.hydration_pct) / 50.0, 0.0, 1.0);
    double energy_depletion_risk = 1.0 - energy_T;
    double R_metabolic = 0.4 * dehydration_risk + 0.6 * energy_depletion_risk;

    double R_thermal = std::max(0.0, (m.temp_celsius - 38.5) / 2.0);

    return Utils::clamp(0.6*R_critical + 0.3*R_metabolic + 0.1*R_thermal, 0.0, 1.0);
}

PatientState StateEstimator::estimate(const Telemetry& m, const PatientProfile& profile,
                     double current_infusion_rate) {
    PatientState state;

    state.hydration_pct = Utils::clamp(m.hydration_pct, 0.0, 100.0);
    state.heart_rate_bpm = std::max(0.0, m.heart_rate_bpm);
    state.coherence_sigma = calculate_coherence(m);

#ifdef ENABLE_NEURAL_ESTIMATOR
    init_neural_estimator();
    if (g_neural_estimator.is_loaded()) {
        state.energy_T = static_cast<double>(g_neural_estimator.predict(
            static_cast<float>(m.hydration_pct  / 100.0),
            static_cast<float>(m.heart_rate_bpm / 200.0),
            static_cast<float>(m.spo2_pct       / 100.0),
            static_cast<float>(m.lactate_mmol   /  20.0),
            static_cast<float>(m.fatigue_idx)));
    } else {
        state.energy_T = calculate_energy_proxy(m);
    }
#else
    state.energy_T = calculate_energy_proxy(m);
#endif

    state.energy_T_absolute = calculate_energy_transfer_absolute(
        m, profile.energy_params, current_infusion_rate,
        profile.weight_kg, profile.current_tissue_perfusion);

    state.estimated_flow_velocity_cm_s = estimate_flow_velocity(
        m, current_infusion_rate, profile.weight_kg);
    state.flow_efficiency = Utils::gaussian(
        state.estimated_flow_velocity_cm_s,
        profile.energy_params.v_optimal_cm_s,
        profile.energy_params.sigma_velocity);

    state.metabolic_load = calculate_metabolic_load(m);
    state.cardiac_reserve = calculate_cardiac_reserve(m, profile.age_years);
    state.risk_score = calculate_risk_score(m, state.energy_T);

    state.uncertainty = 1.0 - (state.coherence_sigma * (1.0 - 0.3*state.metabolic_load));

    history.push_back(state);
    telemetry_history.push_back(m);
    if (history.size() > MAX_HISTORY) {
        history.pop_front();
        telemetry_history.pop_front();
    }

    return state;
}

std::optional<PatientState> StateEstimator::predict_forward(int minutes_ahead) {
    if (history.size() < 5) return std::nullopt;

    PatientState predicted = history.back();

    size_t history_idx = history.size() - 5;
    double hydration_trend = (history.back().hydration_pct -
                             history[history_idx].hydration_pct) / 5.0;
    double energy_trend = (history.back().energy_T -
                          history[history_idx].energy_T) / 5.0;

    predicted.hydration_pct += hydration_trend * minutes_ahead;
    predicted.energy_T += energy_trend * minutes_ahead;
    predicted.hydration_pct = Utils::clamp(predicted.hydration_pct, 0.0, 100.0);
    predicted.energy_T = Utils::clamp(predicted.energy_T, 0.0, 1.0);

    predicted.uncertainty = std::min(1.0, predicted.uncertainty + 0.05 * minutes_ahead);

    return predicted;
}

const std::deque<PatientState>& StateEstimator::get_history() const { return history; }

} // namespace ivsys
