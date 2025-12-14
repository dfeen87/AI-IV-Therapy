/*
 * AI-Optimized Intravenous Therapy Control System
 * Enhanced with Full Energy Transfer Model from White Paper Section 4.1
 * 
 * Updates:
 * - Complete nonlinear energy transfer dynamics (T(t) equation)
 * - Gaussian velocity optimization for substrate delivery
 * - Patient-specific metabolic parameters
 * - ATP/NAD+ delivery efficiency modeling
 * - Future-ready for energy transfer cell integration
 */

#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>
#include <deque>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <optional>
#include <thread>

// ============================================================================
// CORE DATA STRUCTURES
// ============================================================================

struct Telemetry {
    std::chrono::steady_clock::time_point timestamp;
    double hydration_pct;      // 0-100: body water percentage
    double heart_rate_bpm;     // beats per minute
    double temp_celsius;       // core body temperature
    double blood_loss_idx;     // 0-1: cumulative blood loss estimate
    double fatigue_idx;        // 0-1: muscular/metabolic fatigue
    double anxiety_idx;        // 0-1: stress/anxiety level
    double signal_quality;     // 0-1: sensor reliability metric
    double spo2_pct;          // 0-100: blood oxygen saturation
    double lactate_mmol;      // blood lactate concentration
    double cardiac_output_L_min;  // NEW: measured/estimated cardiac output
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

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

class Utils {
public:
    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }
    
    static double sigmoid(double x, double center = 0.0, double steepness = 1.0) {
        return 1.0 / (1.0 + std::exp(-steepness * (x - center)));
    }
    
    static double exponential_decay(double x, double rate = 1.0) {
        return std::exp(-rate * x);
    }
    
    // NEW: Gaussian function for velocity optimization
    static double gaussian(double x, double center, double sigma) {
        double z = (x - center) / sigma;
        return std::exp(-0.5 * z * z);
    }
    
    static std::string timestamp_str(std::chrono::steady_clock::time_point t) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            t.time_since_epoch()).count();
        auto secs = ms / 1000;
        auto millis = ms % 1000;
        
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(6) << secs 
            << "." << std::setw(3) << millis;
        return oss.str();
    }
};

// ============================================================================
// SAFETY CONSTRAINT SYSTEM
// ============================================================================

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
    SafetyMonitor(const PatientProfile& prof) 
        : profile(prof), cumulative_volume_ml(0.0), last_check(std::chrono::steady_clock::now()) {
        max_volume_24h_ml = profile.weight_kg * 35.0;
        if (profile.cardiac_condition) max_volume_24h_ml *= 0.7;
        if (profile.renal_impairment) max_volume_24h_ml *= 0.6;
    }
    
    struct SafetyCheck {
        bool passed;
        double max_allowed_rate;
        std::string warnings;
    };
    
    SafetyCheck evaluate(double requested_rate, const PatientState& state) {
        SafetyCheck result;
        result.passed = true;
        result.max_allowed_rate = profile.max_safe_infusion_rate;
        std::ostringstream warnings;
        
        // Check 1: Volume overload
        auto now = std::chrono::steady_clock::now();
        auto elapsed_h = std::chrono::duration<double, std::ratio<3600>>(
            now - last_check).count();
        
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
    
    void update_volume(double rate_ml_per_min, double duration_min) {
        cumulative_volume_ml += rate_ml_per_min * duration_min;
        recent_rates.push_back(rate_ml_per_min);
        if (recent_rates.size() > 20) recent_rates.pop_front();
    }
    
    void reset_24h_counter() {
        cumulative_volume_ml = 0.0;
    }
    
    double get_cumulative_volume() const { return cumulative_volume_ml; }
};

// ============================================================================
// ENHANCED STATE ESTIMATION WITH FULL ENERGY TRANSFER MODEL
// ============================================================================

class StateEstimator {
private:
    std::deque<PatientState> history;
    std::deque<Telemetry> telemetry_history;
    const size_t MAX_HISTORY = 50;
    
    double calculate_coherence(const Telemetry& m) {
        double base_coherence = m.signal_quality;
        
        if (m.heart_rate_bpm < 40 || m.heart_rate_bpm > 180) base_coherence *= 0.5;
        if (m.temp_celsius < 35.0 || m.temp_celsius > 40.0) base_coherence *= 0.7;
        if (m.spo2_pct < 85.0) base_coherence *= 0.6;
        
        if (telemetry_history.size() >= 5) {
            double hr_variance = 0.0;
            for (size_t i = telemetry_history.size() - 5; i < telemetry_history.size(); ++i) {
                hr_variance += std::pow(telemetry_history[i].heart_rate_bpm - m.heart_rate_bpm, 2);
            }
            hr_variance /= 5.0;
            
            if (hr_variance > 400.0) base_coherence *= 0.7;
        }
        
        return Utils::clamp(base_coherence, 0.1, 1.0);
    }
    
    // NEW: Calculate flow velocity from cardiac output and infusion rate
    double estimate_flow_velocity(const Telemetry& m, double infusion_rate_ml_min, 
                                   double weight_kg) {
        // Simplified model: velocity proportional to cardiac output and infusion
        // Typical blood flow velocity in capillaries: 0.03-0.07 cm/s
        // In larger vessels: 20-40 cm/s
        
        // Assume weighted average across circulation
        double cardiac_contribution = m.cardiac_output_L_min * 4.0; // Scale factor
        double infusion_contribution = infusion_rate_ml_min * 0.5;
        
        double v_estimated = (cardiac_contribution + infusion_contribution) / weight_kg;
        return Utils::clamp(v_estimated, 5.0, 40.0); // Physiological bounds
    }
    
    // NEW: Calculate tissue absorption efficiency based on perfusion state
    double calculate_tissue_efficiency(const Telemetry& m, 
                                       const EnergyTransferParams& params,
                                       double perfusion_state) {
        // Weighted by tissue perfusion and oxygenation
        double base_efficiency = params.eta_muscle;
        
        // Adjust for hypoxia/ischemia
        if (m.spo2_pct < 90.0) {
            double hypoxia_penalty = (90.0 - m.spo2_pct) / 20.0;
            base_efficiency *= (1.0 - Utils::clamp(hypoxia_penalty, 0.0, 0.6));
        }
        
        // Adjust for perfusion state
        base_efficiency *= (0.5 + 0.5 * perfusion_state);
        
        // Temperature effects (hypothermia reduces efficiency)
        if (m.temp_celsius < 36.0) {
            double temp_penalty = (36.0 - m.temp_celsius) / 5.0;
            base_efficiency *= (1.0 - Utils::clamp(temp_penalty, 0.0, 0.4));
        }
        
        return Utils::clamp(base_efficiency, params.eta_ischemic, params.eta_brain_heart);
    }
    
    // NEW: Full energy transfer model T(t) from white paper
    double calculate_energy_transfer_absolute(const Telemetry& m,
                                               const EnergyTransferParams& params,
                                               double infusion_rate_ml_min,
                                               double weight_kg,
                                               double perfusion_state) {
        // T(t) = [P_input(t) · G(v, v₀, σ) · ṁ(t) · I_sp · η_tissue] / M_patient
        
        // Total metabolic power input
        double P_input = params.P_baseline + params.P_iv_supplement + params.P_energy_cells;
        
        // Flow velocity and Gaussian optimization
        double v = estimate_flow_velocity(m, infusion_rate_ml_min, weight_kg);
        double G_v = Utils::gaussian(v, params.v_optimal_cm_s, params.sigma_velocity);
        
        // Mass flow rate (convert ml/min to kg/s, assuming density ~1.0)
        double m_dot = infusion_rate_ml_min / 60000.0; // kg/s
        
        // Specific energy delivery (currently standard IV)
        double I_sp = params.I_sp_standard;
        
        // Tissue absorption efficiency
        double eta = calculate_tissue_efficiency(m, params, perfusion_state);
        
        // Complete energy transfer equation
        double T_t = (P_input * G_v * m_dot * I_sp * eta) / weight_kg;
        
        return T_t; // Returns W/kg
    }
    
    // Enhanced energy proxy E_T from white paper section 4.1.2
    double calculate_energy_proxy(const Telemetry& m) {
        // Hydration term (30% weight, sigmoid at 60%)
        double h_term = Utils::sigmoid(m.hydration_pct, 60.0, 0.1);
        
        // Blood loss term (25% weight, exponential penalty)
        // 40% blood loss → 95% reduction (catastrophic)
        double b_term = Utils::exponential_decay(m.blood_loss_idx, 3.0);
        
        // Fatigue term (20% weight, cliff at 70%)
        // Above 70%: 70% penalty (glycogen depletion)
        double f_term;
        if (m.fatigue_idx < 0.7) {
            f_term = 1.0 - m.fatigue_idx;
        } else {
            f_term = 0.3 * (1.0 - m.fatigue_idx);  // Catastrophic drop
        }
        
        // Oxygen term (15% weight, sigmoid at 92% SpO2)
        double o_term = Utils::sigmoid(m.spo2_pct, 92.0, 0.3);
        
        // Lactate term (10% weight, exponential above 2 mmol/L baseline)
        double l_term = Utils::exponential_decay(std::max(0.0, m.lactate_mmol - 2.0), 0.5);
        
        // Weighted combination (from white paper)
        double energy = 0.30 * h_term + 0.25 * b_term + 0.20 * f_term + 
                       0.15 * o_term + 0.10 * l_term;
        
        return Utils::clamp(energy, 0.0, 1.0);
    }
    
    double calculate_metabolic_load(const Telemetry& m) {
        double hr_stress = Utils::clamp((m.heart_rate_bpm - 60.0) / 100.0, 0.0, 1.0);
        double temp_stress = std::abs(m.temp_celsius - 37.0) / 3.0;
        double lactate_stress = Utils::clamp(m.lactate_mmol / 10.0, 0.0, 1.0);
        double anxiety_stress = m.anxiety_idx;
        
        return Utils::clamp(0.3*hr_stress + 0.25*temp_stress + 
                           0.25*lactate_stress + 0.2*anxiety_stress, 0.0, 1.0);
    }
    
    double calculate_cardiac_reserve(const Telemetry& m, double baseline_hr) {
        // Age-adjusted max HR (rough approximation from baseline)
        double age_estimate = (220.0 - baseline_hr) / 0.7; // Reverse engineer
        double max_predicted_hr = 220.0 - age_estimate;
        double current_percentage = m.heart_rate_bpm / max_predicted_hr;
        
        // Reserve decreases nonlinearly at 85% threshold
        double reserve = 1.0 - Utils::sigmoid(current_percentage, 0.85, 10.0);
        
        // Adjust for SpO2
        reserve *= Utils::clamp(m.spo2_pct / 95.0, 0.5, 1.0);
        
        return Utils::clamp(reserve, 0.0, 1.0);
    }
    
    double calculate_risk_score(const Telemetry& m, double energy_T) {
        // Critical factors (max operation - any one triggers high risk)
        double blood_loss_risk = m.blood_loss_idx;
        double hypoxia_risk = Utils::clamp((95.0 - m.spo2_pct) / 10.0, 0.0, 1.0);
        double hypothermia_risk = std::max(0.0, (36.0 - m.temp_celsius) / 2.0);
        
        double R_critical = std::max({blood_loss_risk, hypoxia_risk, hypothermia_risk});
        
        // Metabolic factors
        double dehydration_risk = Utils::clamp((100.0 - m.hydration_pct) / 50.0, 0.0, 1.0);
        double energy_depletion_risk = 1.0 - energy_T;
        double R_metabolic = 0.4 * dehydration_risk + 0.6 * energy_depletion_risk;
        
        // Thermal stress
        double R_thermal = std::max(0.0, (m.temp_celsius - 38.5) / 2.0);
        
        // Composite (from white paper: 60% critical, 30% metabolic, 10% thermal)
        return Utils::clamp(0.6*R_critical + 0.3*R_metabolic + 0.1*R_thermal, 0.0, 1.0);
    }
    
public:
    PatientState estimate(const Telemetry& m, const PatientProfile& profile, 
                         double current_infusion_rate) {
        PatientState state;
        
        state.hydration_pct = Utils::clamp(m.hydration_pct, 0.0, 100.0);
        state.heart_rate_bpm = std::max(0.0, m.heart_rate_bpm);
        state.coherence_sigma = calculate_coherence(m);
        
        // Energy proxy (normalized 0-1)
        state.energy_T = calculate_energy_proxy(m);
        
        // Absolute energy transfer (W/kg) - for advanced analysis
        state.energy_T_absolute = calculate_energy_transfer_absolute(
            m, profile.energy_params, current_infusion_rate, 
            profile.weight_kg, profile.current_tissue_perfusion);
        
        // Flow dynamics
        state.estimated_flow_velocity_cm_s = estimate_flow_velocity(
            m, current_infusion_rate, profile.weight_kg);
        state.flow_efficiency = Utils::gaussian(
            state.estimated_flow_velocity_cm_s,
            profile.energy_params.v_optimal_cm_s,
            profile.energy_params.sigma_velocity);
        
        state.metabolic_load = calculate_metabolic_load(m);
        state.cardiac_reserve = calculate_cardiac_reserve(m, profile.baseline_hr_bpm);
        state.risk_score = calculate_risk_score(m, state.energy_T);
        
        // Uncertainty increases with low coherence and high metabolic load
        state.uncertainty = 1.0 - (state.coherence_sigma * (1.0 - 0.3*state.metabolic_load));
        
        // Store in history
        history.push_back(state);
        telemetry_history.push_back(m);
        if (history.size() > MAX_HISTORY) {
            history.pop_front();
            telemetry_history.pop_front();
        }
        
        return state;
    }
    
    std::optional<PatientState> predict_forward(int minutes_ahead) {
        if (history.size() < 5) return std::nullopt;
        
        PatientState predicted = history.back();
        
        // Linear extrapolation with uncertainty growth
        double hydration_trend = (history.back().hydration_pct - 
                                 history[history.size()-5].hydration_pct) / 5.0;
        double energy_trend = (history.back().energy_T - 
                              history[history.size()-5].energy_T) / 5.0;
        
        predicted.hydration_pct += hydration_trend * minutes_ahead;
        predicted.energy_T += energy_trend * minutes_ahead;
        
        // Increase uncertainty (5% per minute as per white paper)
        predicted.uncertainty = std::min(1.0, predicted.uncertainty + 0.05 * minutes_ahead);
        
        return predicted;
    }
    
    const std::deque<PatientState>& get_history() const { return history; }
};

// ============================================================================
// AI CONTROL ALGORITHM (Enhanced)
// ============================================================================

class AdaptiveController {
private:
    PatientProfile profile;
    double last_command;
    
    double calculate_base_rate(const PatientState& state) {
        // Hydration urgency (nonlinear - sigmoid ramp above 50% deficit)
        double hydration_deficit = (100.0 - state.hydration_pct) / 100.0;
        double hydration_urgency = hydration_deficit < 0.5 ? 
            hydration_deficit : Utils::sigmoid(hydration_deficit, 0.5, 5.0);
        
        // Energy gap with metabolic amplification
        double energy_gap = 1.0 - state.energy_T;
        double energy_need = energy_gap * (1.0 + 0.5 * state.metabolic_load);
        
        // Risk amplification (up to 50% increase)
        double risk_amplifier = 1.0 + 0.5 * state.risk_score;
        
        // Base calculation: 0.4-1.8 ml/min range (from white paper)
        double base = 0.4 + 1.4 * Utils::clamp(
            (0.6 * hydration_urgency + 0.4 * energy_need) * risk_amplifier,
            0.0, 1.0
        );
        
        return base;
    }
    
    double apply_coherence_modulation(double base_rate, const PatientState& state) {
        // Direct scaling by coherence (from white paper section 5.1)
        return base_rate * state.coherence_sigma;
    }
    
    double apply_cardiac_limiting(double rate, const PatientState& state) {
        // f_cardiac function from white paper
        if (state.cardiac_reserve < 0.3) {
            double scaling = 0.5 + 0.5 * Utils::sigmoid(state.cardiac_reserve, 0.3, 10.0);
            return rate * scaling;
        }
        return rate;
    }
    
    std::string generate_rationale(const PatientState& state, double rate, 
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
    
public:
    AdaptiveController(const PatientProfile& prof) 
        : profile(prof), last_command(0.4) {}
    
    ControlOutput decide(const PatientState& state, SafetyMonitor& safety,
                        StateEstimator& estimator) {
        ControlOutput output;
        bool predictive_boost = false;
        
        // Step 1: Calculate desired rate
        double desired_rate = calculate_base_rate(state);
        
        // Step 2: Predictive control (from white paper section 5.2)
        auto predicted = estimator.predict_forward(10); // 10 min horizon
        if (predicted.has_value() && predicted->hydration_pct < 50.0) {
            desired_rate *= 1.2; // 20% preemptive increase
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
};

// ============================================================================
// LOGGING AND MONITORING
// ============================================================================

class SystemLogger {
private:
    std::ofstream log_file;
    std::ofstream telemetry_file;
    std::ofstream control_file;
    
public:
    SystemLogger(const std::string& session_id) {
        std::string prefix = "ai_iv_" + session_id;
        log_file.open(prefix + "_system.log");
        telemetry_file.open(prefix + "_telemetry.csv");
        control_file.open(prefix + "_control.csv");
        
        telemetry_file << "timestamp,hydration_pct,heart_rate_bpm,temp_c,blood_loss_idx,"
                       << "fatigue_idx,anxiety_idx,signal_quality,spo2_pct,lactate_mmol,"
                       << "cardiac_output_L_min\n";
        
        control_file << "timestamp,infusion_rate_ml_min,confidence,energy_T,energy_T_abs_W_kg,"
                    << "flow_velocity_cm_s,flow_efficiency,risk_score,"
                    << "cardiac_reserve,warnings,rationale\n";
    }
    
    void log_telemetry(const Telemetry& m) {
        telemetry_file << Utils::timestamp_str(m.timestamp) << ","
                       << m.hydration_pct << ","
                       << m.heart_rate_bpm << ","
                       << m.temp_celsius << ","
                       << m.blood_loss_idx << ","
                       << m.fatigue_idx << ","
                       << m.anxiety_idx << ","
                       << m.signal_quality << ","
                       << m.spo2_pct << ","
                       << m.lactate_mmol << ","
                       << m.cardiac_output_L_min << "\n";
        telemetry_file.flush();
    }
    
    void log_control(const ControlOutput& out, const PatientState& state,
                     std::chrono::steady_clock::time_point t) {
        control_file << Utils::timestamp_str(t) << ","
                     << out.infusion_ml_per_min << ","
                     << out.confidence << ","
                     << state.energy_T << ","
                     << state.energy_T_absolute << ","
                     << state.estimated_flow_velocity_cm_s << ","
                     << state.flow_efficiency << ","
                     << state.risk_score << ","
                     << state.cardiac_reserve << ","
                     << out.warning_flags << ","
                     << out.rationale << "\n";
        control_file.flush();
    }
    
    void log_event(const std::string& event) {
        auto now = std::chrono::steady_clock::now();
        log_file << "[" << Utils::timestamp_str(now) << "] " << event << "\n";
        log_file.flush();
    }
    
    ~SystemLogger() {
        log_file.close();
        telemetry_file.close();
        control_file.close();
    }
};

// ============================================================================
// MAIN CONTROL LOOP
// ============================================================================

class AIIVSystem {
private:
    PatientProfile profile;
    StateEstimator estimator;
    AdaptiveController controller;
    SafetyMonitor safety;
    SystemLogger logger;
    
    bool running;
    double current_infusion_rate;
    const std::chrono::milliseconds control_period{200};  // 5 Hz
    
public:
    AIIVSystem(const PatientProfile& prof, const std::string& session_id)
        : profile(prof), controller(prof), safety(prof), logger(session_id), 
          running(false), current_infusion_rate(0.4) {
        logger.log_event("System initialized - Enhanced Energy Transfer Model v1.0");
        logger.log_event("Patient: " + std::to_string(prof.weight_kg) + "kg, " + 
                        std::to_string(prof.age_years) + "y");
        logger.log_event("Optimal flow velocity: " + 
                        std::to_string(prof.energy_params.v_optimal_cm_s) + " cm/s");
    }
    
    void start() {
        running = true;
        logger.log_event("Control loop started");
        
        auto next_tick = std::chrono::steady_clock::now();
        
        while (running) {
            // 1. Acquire telemetry
            Telemetry measurement = acquire_telemetry();
            
            // 2. State estimation with energy transfer model
            PatientState state = estimator.estimate(measurement, profile, current_infusion_rate);
            
            // 3. Control decision with predictive capability
            ControlOutput command = controller.decide(state, safety, estimator);
            
            // 4. Update current rate for next cycle
            current_infusion_rate = command.infusion_ml_per_min;
            
            // 5. Logging
            logger.log_telemetry(measurement);
            logger.log_control(command, state, measurement.timestamp);
            
            // 6. Display status
            display_status(state, command);
            
            // 7. Update safety monitor
            double cycle_duration_min = control_period.count() / 60000.0;
            safety.update_volume(command.infusion_ml_per_min, cycle_duration_min);
            
            // 8. Send command to infusion pump (placeholder)
            // send_to_pump(command.infusion_ml_per_min);
            
            // 9. Timing
            next_tick += control_period;
            std::this_thread::sleep_until(next_tick);
        }
        
        logger.log_event("Control loop stopped");
    }
    
    void stop() {
        running = false;
    }
    
private:
    Telemetry acquire_telemetry() {
        // SIMULATION: Replace with actual sensor interface
        static double sim_time = 0.0;
        sim_time += control_period.count() / 1000.0;
        
        Telemetry m;
        m.timestamp = std::chrono::steady_clock::now();
        
        // Simulate gradual dehydration and recovery pattern
        m.hydration_pct = 65.0 + 15.0 * std::sin(sim_time * 0.05);
        m.heart_rate_bpm = profile.baseline_hr_bpm + 20.0 * std::sin(sim_time * 0.1);
        m.temp_celsius = 37.0 + 0.5 * std::sin(sim_time * 0.03);
        m.blood_loss_idx = 0.0;
        m.fatigue_idx = 0.3 + 0.2 * std::sin(sim_time * 0.02);
        m.anxiety_idx = 0.2;
        m.signal_quality = 0.85 + 0.1 * std::sin(sim_time * 0.5);
        m.spo2_pct = 97.0 + 2.0 * std::sin(sim_time * 0.08);
        m.lactate_mmol = 2.0 + 1.0 * std::sin(sim_time * 0.04);
        
        // NEW: Simulated cardiac output (L/min)
        m.cardiac_output_L_min = 5.0 + 1.0 * std::sin(sim_time * 0.06);
        
        return m;
    }
    
    void display_status(const PatientState& state, const ControlOutput& cmd) {
        static int display_counter = 0;
        if (++display_counter % 10 == 0) {  // Update display every 2 seconds
            std::cout << "\n=== AI-IV Enhanced Energy Transfer System ===\n";
            std::cout << "Hydration: " << std::fixed << std::setprecision(1) 
                     << state.hydration_pct << "%  ";
            std::cout << "Energy_T: " << std::setprecision(3) << state.energy_T << "  ";
            std::cout << "HR: " << std::setprecision(0) << state.heart_rate_bpm << " bpm\n";
            
            std::cout << "Energy Transfer: " << std::setprecision(2) 
                     << state.energy_T_absolute << " W/kg  ";
            std::cout << "Flow: " << state.estimated_flow_velocity_cm_s << " cm/s  ";
            std::cout << "G(v): " << std::setprecision(3) << state.flow_efficiency << "\n";
            
            std::cout << "Risk: " << std::setprecision(2) << state.risk_score << "  ";
            std::cout << "Cardiac Reserve: " << state.cardiac_reserve << "  ";
            std::cout << "Coherence: " << state.coherence_sigma << "\n";
            
            std::cout << "Infusion Rate: " << std::setprecision(2) 
                     << cmd.infusion_ml_per_min << " ml/min  ";
            std::cout << "Confidence: " << cmd.confidence << "\n";
            
            if (!cmd.warning_flags.empty()) {
                std::cout << "⚠️  WARNINGS: " << cmd.warning_flags << "\n";
            }
            std::cout << "24h Volume: " << std::setprecision(0) 
                     << safety.get_cumulative_volume() << " ml\n";
            std::cout << std::flush;
        }
    }
};

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  AI-IV Control System v2.0 - Enhanced Energy Transfer     ║\n";
    std::cout << "║  Full nonlinear dynamics from white paper section 4.1     ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    // Define patient profile
    PatientProfile patient;
    patient.weight_kg = 75.0;
    patient.age_years = 35.0;
    patient.cardiac_condition = false;
    patient.renal_impairment = false;
    patient.diabetes = false;
    patient.baseline_hr_bpm = 70.0;
    patient.max_safe_infusion_rate = 1.5;
    patient.current_tissue_perfusion = 0.85; // Good perfusion
    
    // Energy transfer parameters (defaults to standard IV therapy)
    patient.energy_params = EnergyTransferParams();
    
    std::string session_id = std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    
    std::cout << "Patient Profile:\n";
    std::cout << "  Weight: " << patient.weight_kg << " kg\n";
    std::cout << "  Age: " << patient.age_years << " years\n";
    std::cout << "  Baseline HR: " << patient.baseline_hr_bpm << " bpm\n";
    std::cout << "  Max Infusion Rate: " << patient.max_safe_infusion_rate << " ml/min\n\n";
    
    std::cout << "Energy Transfer Parameters:\n";
    std::cout << "  P_baseline: " << patient.energy_params.P_baseline << " W\n";
    std::cout << "  P_IV_supplement: " << patient.energy_params.P_iv_supplement << " W\n";
    std::cout << "  I_sp (standard): " << patient.energy_params.I_sp_standard << " kJ/kg\n";
    std::cout << "  v_optimal: " << patient.energy_params.v_optimal_cm_s << " cm/s\n";
    std::cout << "  σ_velocity: " << patient.energy_params.sigma_velocity << " cm/s\n";
    std::cout << "  Tissue perfusion: " << patient.current_tissue_perfusion << "\n\n";
    
    std::cout << "Session ID: " << session_id << "\n";
    std::cout << "Log files: ai_iv_" << session_id << "_*.{log,csv}\n\n";
    
    AIIVSystem system(patient, session_id);
    
    std::cout << "Starting control loop (press Ctrl+C to stop)...\n\n";
    
    std::thread control_thread([&system]() {
        system.start();
    });
    
    // Demo: run for 60 seconds
    std::this_thread::sleep_for(std::chrono::seconds(60));
    
    std::cout << "\n\nStopping system...\n";
    system.stop();
    control_thread.join();
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  System shutdown complete - check logs for full data      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    
    return 0;
}
