/*
 * iv_system_types.hpp
 *
 * Public system data contracts for the AI-Optimized Intravenous Therapy
 * Control System.
 */

#pragma once

#include <string>
#include <chrono>
#include <optional>
#include <deque>
#include <vector>
#include <cmath>

namespace ivsys {

// ============================================================================
// CORE DATA STRUCTURES
// ============================================================================

struct Telemetry {
    std::chrono::steady_clock::time_point timestamp = std::chrono::steady_clock::now();
    double hydration_pct = 0.0;      // 0-100: body water percentage
    double heart_rate_bpm = 0.0;     // beats per minute
    double temp_celsius = 0.0;       // core body temperature
    double blood_loss_idx = 0.0;     // 0-1: cumulative blood loss estimate
    double fatigue_idx = 0.0;        // 0-1: muscular/metabolic fatigue
    double anxiety_idx = 0.0;        // 0-1: stress/anxiety level
    double signal_quality = 0.0;     // 0-1: sensor reliability metric
    double spo2_pct = 0.0;          // 0-100: blood oxygen saturation
    double lactate_mmol = 0.0;      // blood lactate concentration
    double cardiac_output_L_min = 0.0;  // NEW: measured/estimated cardiac output
};

struct EnergyTransferParams {
    // Metabolic power generation (Watts)
    double P_baseline;         // Baseline cellular respiration (~100W)
    double P_iv_supplement;    // IV substrate supplementation (+20-50W)
    double P_energy_cells;     // Energy transfer cells (future: +50-100W)

    // Specific energy delivery (kJ/kg)
    double I_sp_standard;      // Standard IV fluids (~1.2 kJ/kg)
    double I_sp_atp_loaded;    // ATP-loaded vesicles (~4.5 kJ/kg)
    double I_sp_mitochondrial; // Mitochondrial transplant (~8.0 kJ/kg)

    // Tissue absorption efficiency (dimensionless)
    double eta_brain_heart;    // Well-perfused: 0.85-0.95
    double eta_muscle;         // Muscle: 0.70-0.80
    double eta_ischemic;       // Ischemic/hypoxic: 0.30-0.50

    // Flow velocity optimization
    double v_optimal_cm_s;     // Patient-specific optimal velocity (15-25 cm/s)
    double sigma_velocity;     // Velocity tolerance (narrow for critical, wide for healthy)

    EnergyTransferParams() {
        // Defaults for standard IV therapy (no energy cells yet)
        P_baseline = 100.0;
        P_iv_supplement = 35.0;  // Mid-range
        P_energy_cells = 0.0;    // Not yet deployed

        I_sp_standard = 1.2;
        I_sp_atp_loaded = 4.5;
        I_sp_mitochondrial = 8.0;

        eta_brain_heart = 0.90;
        eta_muscle = 0.75;
        eta_ischemic = 0.40;

        v_optimal_cm_s = 20.0;   // Mid-range optimal
        sigma_velocity = 5.0;     // Moderate tolerance
    }
};

struct PatientState {
    double hydration_pct;
    double heart_rate_bpm;
    double coherence_sigma;    // temporal stabilizer (0-1)
    double energy_T;           // ATP/metabolic energy proxy (0-1)
    double energy_T_absolute;  // NEW: Absolute energy transfer (W/kg)
    double metabolic_load;     // stress on metabolic systems (0-1)
    double cardiac_reserve;    // heart capacity headroom (0-1)
    double risk_score;         // composite risk metric (0-1)

    // NEW: Flow dynamics
    double estimated_flow_velocity_cm_s;
    double flow_efficiency;    // G(v) - Gaussian term

    // Prediction uncertainty
    double uncertainty;        // confidence in state estimate (0-1)
};

struct ControlOutput {
    double infusion_ml_per_min;
    double confidence;
    std::string rationale;
    bool safety_override;
    std::string warning_flags;
};

struct PatientProfile {
    double weight_kg;
    double age_years;
    bool cardiac_condition;
    bool renal_impairment;
    bool diabetes;
    double baseline_hr_bpm;
    double max_safe_infusion_rate;

    // NEW: Energy transfer specific
    EnergyTransferParams energy_params;
    double current_tissue_perfusion; // 0-1: overall perfusion state
};

enum class AlertSeverity {
    Debug,
    Info,
    Warn,
    Error,
    Critical
};

struct AlertEvent {
    long long timestamp_ms;
    AlertSeverity severity;
    std::string source;
    std::string code;
    std::string message;
    std::optional<std::string> context_json;
};

} // namespace ivsys
