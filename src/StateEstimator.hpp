#pragma once

#include "iv_system_types.hpp"
#include <deque>
#include <optional>

namespace ivsys {

class StateEstimator {
private:
    std::deque<PatientState> history;
    std::deque<Telemetry> telemetry_history;
    const size_t MAX_HISTORY = 50;

    double calculate_coherence(const Telemetry& m);
    double estimate_flow_velocity(const Telemetry& m, double infusion_rate_ml_min, double weight_kg);
    double calculate_tissue_efficiency(const Telemetry& m, const EnergyTransferParams& params, double perfusion_state);
    double calculate_energy_transfer_absolute(const Telemetry& m, const EnergyTransferParams& params, double infusion_rate_ml_min, double weight_kg, double perfusion_state);
    double calculate_energy_proxy(const Telemetry& m);
    double calculate_metabolic_load(const Telemetry& m);
    double calculate_cardiac_reserve(const Telemetry& m, double age_years);
    double calculate_risk_score(const Telemetry& m, double energy_T);

public:
    PatientState estimate(const Telemetry& m, const PatientProfile& profile, double current_infusion_rate);
    std::optional<PatientState> predict_forward(int minutes_ahead);
    const std::deque<PatientState>& get_history() const;
};

} // namespace ivsys
