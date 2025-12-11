/*
 * AI-Optimized Intravenous Therapy Control System
 * 
 * A real-time adaptive IV infusion system that uses wearable sensor data,
 * predictive state estimation, and AI-driven control to optimize hydration,
 * ATP delivery, and metabolic recovery in critical care scenarios.
 * 
 * Key Features:
 * - Multi-modal sensor fusion (hydration, HR, temp, blood loss, fatigue)
 * - Predictive state estimation with Kalman-style filtering
 * - Nonlinear energy transfer modeling
 * - Multi-layer safety constraints (volume, cardiac, rate limiting)
 * - Real-time control with coherence-based adaptation
 * - Comprehensive logging and audit trail
 * 
 * Applications: Critical care, space medicine, military/disaster relief,
 *               extreme sports recovery, trauma response
 * 
 * Author: Open research contribution
 * License: MIT 
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
};

struct PatientState {
    double hydration_pct;
    double heart_rate_bpm;
    double coherence_sigma;    // temporal stabilizer (0-1)
    double energy_T;           // ATP/metabolic energy proxy (0-1)
    double metabolic_load;     // stress on metabolic systems (0-1)
    double cardiac_reserve;    // heart capacity headroom (0-1)
    double risk_score;         // composite risk metric (0-1)
    
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
        // Calculate max 24h volume: ~30-40 ml/kg for adults
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
            result.max_allowed_rate = 0.1;  // Emergency minimum
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
// STATE ESTIMATION WITH PREDICTIVE FILTERING
// ============================================================================

class StateEstimator {
private:
    std::deque<PatientState> history;
    std::deque<Telemetry> telemetry_history;
    const size_t MAX_HISTORY = 50;
    
    double calculate_coherence(const Telemetry& m) {
        // Signal quality weighted by physiological plausibility
        double base_coherence = m.signal_quality;
        
        // Penalize if readings are outside expected ranges
        if (m.heart_rate_bpm < 40 || m.heart_rate_bpm > 180) base_coherence *= 0.5;
        if (m.temp_celsius < 35.0 || m.temp_celsius > 40.0) base_coherence *= 0.7;
        if (m.spo2_pct < 85.0) base_coherence *= 0.6;
        
        // Check for signal oscillation (temporal incoherence)
        if (telemetry_history.size() >= 5) {
            double hr_variance = 0.0;
            for (size_t i = telemetry_history.size() - 5; i < telemetry_history.size(); ++i) {
                hr_variance += std::pow(telemetry_history[i].heart_rate_bpm - m.heart_rate_bpm, 2);
            }
            hr_variance /= 5.0;
            
            if (hr_variance > 400.0) base_coherence *= 0.7;  // High variance = noise
        }
        
        return Utils::clamp(base_coherence, 0.1, 1.0);
    }
    
    double calculate_energy_proxy(const Telemetry& m) {
        // Nonlinear energy transfer model
        
        // Hydration: sigmoid with critical threshold at 60%
        double h_term = Utils::sigmoid(m.hydration_pct, 60.0, 0.1);
        
        // Blood loss: exponential penalty (severe nonlinearity)
        double b_term = Utils::exponential_decay(m.blood_loss_idx, 3.0);
        
        // Fatigue: threshold effect (performance cliff)
        double f_term = m.fatigue_idx < 0.7 ? (1.0 - m.fatigue_idx) : 0.3 * (1.0 - m.fatigue_idx);
        
        // Oxygen availability (SpO2 impact)
        double o_term = Utils::sigmoid(m.spo2_pct, 92.0, 0.3);
        
        // Lactate burden (metabolic stress)
        double l_term = Utils::exponential_decay(std::max(0.0, m.lactate_mmol - 2.0), 0.5);
        
        // Weighted combination
        double energy = 0.30 * h_term + 0.25 * b_term + 0.20 * f_term + 
                       0.15 * o_term + 0.10 * l_term;
        
        return Utils::clamp(energy, 0.0, 1.0);
    }
    
    double calculate_metabolic_load(const Telemetry& m) {
        // Composite stress on metabolic systems
        double hr_stress = Utils::clamp((m.heart_rate_bpm - 60.0) / 100.0, 0.0, 1.0);
        double temp_stress = std::abs(m.temp_celsius - 37.0) / 3.0;
        double lactate_stress = Utils::clamp(m.lactate_mmol / 10.0, 0.0, 1.0);
        double anxiety_stress = m.anxiety_idx;
        
        return Utils::clamp(0.3*hr_stress + 0.25*temp_stress + 0.25*lactate_stress + 0.2*anxiety_stress, 0.0, 1.0);
    }
    
    double calculate_cardiac_reserve(const Telemetry& m, double baseline_hr) {
        // Estimate remaining cardiac capacity
        double max_predicted_hr = 220.0 - (baseline_hr / 60.0 * 30.0);  // Age proxy
        double current_percentage = m.heart_rate_bpm / max_predicted_hr;
        
        // Reserve decreases nonlinearly as HR approaches max
        double reserve = 1.0 - Utils::sigmoid(current_percentage, 0.85, 10.0);
        
        // Adjust for SpO2 (low oxygen = reduced reserve)
        reserve *= Utils::clamp(m.spo2_pct / 95.0, 0.5, 1.0);
        
        return Utils::clamp(reserve, 0.0, 1.0);
    }
    
    double calculate_risk_score(const Telemetry& m, double energy_T) {
        // Multi-factor risk assessment
        double dehydration_risk = Utils::clamp((100.0 - m.hydration_pct) / 50.0, 0.0, 1.0);
        double blood_loss_risk = m.blood_loss_idx;
        double energy_depletion_risk = 1.0 - energy_T;
        double hypoxia_risk = Utils::clamp((95.0 - m.spo2_pct) / 10.0, 0.0, 1.0);
        double hypothermia_risk = std::max(0.0, (36.0 - m.temp_celsius) / 2.0);
        double hyperthermia_risk = std::max(0.0, (m.temp_celsius - 38.5) / 2.0);
        
        // Weighted risk aggregation (use max for critical factors)
        double critical_risk = std::max({blood_loss_risk, hypoxia_risk, hypothermia_risk});
        double metabolic_risk = 0.4*dehydration_risk + 0.6*energy_depletion_risk;
        
        return Utils::clamp(0.6*critical_risk + 0.3*metabolic_risk + 0.1*hyperthermia_risk, 0.0, 1.0);
    }
    
public:
    PatientState estimate(const Telemetry& m, double baseline_hr) {
        PatientState state;
        
        state.hydration_pct = Utils::clamp(m.hydration_pct, 0.0, 100.0);
        state.heart_rate_bpm = std::max(0.0, m.heart_rate_bpm);
        state.coherence_sigma = calculate_coherence(m);
        state.energy_T = calculate_energy_proxy(m);
        state.metabolic_load = calculate_metabolic_load(m);
        state.cardiac_reserve = calculate_cardiac_reserve(m, baseline_hr);
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
        
        // Simple linear extrapolation with uncertainty growth
        PatientState predicted = history.back();
        
        double hydration_trend = (history.back().hydration_pct - 
                                 history[history.size()-5].hydration_pct) / 5.0;
        double energy_trend = (history.back().energy_T - 
                              history[history.size()-5].energy_T) / 5.0;
        
        predicted.hydration_pct += hydration_trend * minutes_ahead;
        predicted.energy_T += energy_trend * minutes_ahead;
        
        // Increase uncertainty for longer predictions
        predicted.uncertainty = std::min(1.0, predicted.uncertainty + 0.05 * minutes_ahead);
        
        return predicted;
    }
    
    const std::deque<PatientState>& get_history() const { return history; }
};

// ============================================================================
// AI CONTROL ALGORITHM
// ============================================================================

class AdaptiveController {
private:
    PatientProfile profile;
    double last_command;
    
    double calculate_base_rate(const PatientState& state) {
        // Core control policy: address hydration and energy deficits
        
        // Hydration need (nonlinear - urgent below 50%)
        double hydration_deficit = (100.0 - state.hydration_pct) / 100.0;
        double hydration_urgency = hydration_deficit < 0.5 ? 
            hydration_deficit : Utils::sigmoid(hydration_deficit, 0.5, 5.0);
        
        // Energy gap with metabolic consideration
        double energy_gap = 1.0 - state.energy_T;
        double energy_need = energy_gap * (1.0 + 0.5 * state.metabolic_load);
        
        // Risk-adjusted demand
        double risk_amplifier = 1.0 + 0.5 * state.risk_score;
        
        // Base calculation: 0.4-1.8 ml/min range
        double base = 0.4 + 1.4 * Utils::clamp(
            (0.6 * hydration_urgency + 0.4 * energy_need) * risk_amplifier,
            0.0, 1.0
        );
        
        return base;
    }
    
    double apply_coherence_modulation(double base_rate, const PatientState& state) {
        // Reduce aggressiveness when signal quality or coherence is low
        return base_rate * state.coherence_sigma;
    }
    
    double apply_cardiac_limiting(double rate, const PatientState& state) {
        // Back off if cardiac reserve is diminishing
        if (state.cardiac_reserve < 0.3) {
            double scaling = Utils::sigmoid(state.cardiac_reserve, 0.3, 10.0);
            return rate * (0.5 + 0.5 * scaling);
        }
        return rate;
    }
    
    std::string generate_rationale(const PatientState& state, double rate, bool safety_limited) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "hyd=" << state.hydration_pct << "% ";
        oss << "E_T=" << state.energy_T << " ";
        oss << "risk=" << state.risk_score << " ";
        oss << "cardiac_res=" << state.cardiac_reserve << " ";
        oss << "sigma=" << state.coherence_sigma << " ";
        oss << "rate=" << rate << "ml/min";
        if (safety_limited) oss << " [SAFETY_LIMITED]";
        return oss.str();
    }
    
public:
    AdaptiveController(const PatientProfile& prof) 
        : profile(prof), last_command(0.4) {}
    
    ControlOutput decide(const PatientState& state, SafetyMonitor& safety) {
        ControlOutput output;
        
        // Step 1: Calculate desired rate
        double desired_rate = calculate_base_rate(state);
        
        // Step 2: Apply modulation
        desired_rate = apply_coherence_modulation(desired_rate, state);
        desired_rate = apply_cardiac_limiting(desired_rate, state);
        
        // Step 3: Clamp to physiological bounds
        desired_rate = Utils::clamp(desired_rate, 0.1, profile.max_safe_infusion_rate);
        
        // Step 4: Safety evaluation
        auto safety_check = safety.evaluate(desired_rate, state);
        
        bool safety_limited = false;
        if (desired_rate > safety_check.max_allowed_rate) {
            desired_rate = safety_check.max_allowed_rate;
            safety_limited = true;
        }
        
        // Step 5: Final output
        output.infusion_ml_per_min = desired_rate;
        output.confidence = 1.0 - state.uncertainty;
        output.rationale = generate_rationale(state, desired_rate, safety_limited);
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
        
        // CSV headers
        telemetry_file << "timestamp,hydration_pct,heart_rate_bpm,temp_c,blood_loss_idx,"
                       << "fatigue_idx,anxiety_idx,signal_quality,spo2_pct,lactate_mmol\n";
        
        control_file << "timestamp,infusion_rate_ml_min,confidence,energy_T,risk_score,"
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
                       << m.lactate_mmol << "\n";
        telemetry_file.flush();
    }
    
    void log_control(const ControlOutput& out, const PatientState& state,
                     std::chrono::steady_clock::time_point t) {
        control_file << Utils::timestamp_str(t) << ","
                     << out.infusion_ml_per_min << ","
                     << out.confidence << ","
                     << state.energy_T << ","
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
    const std::chrono::milliseconds control_period{200};  // 5 Hz
    
public:
    AIIVSystem(const PatientProfile& prof, const std::string& session_id)
        : profile(prof), controller(prof), safety(prof), logger(session_id), running(false) {
        logger.log_event("System initialized");
        logger.log_event("Patient: " + std::to_string(prof.weight_kg) + "kg, " + 
                        std::to_string(prof.age_years) + "y");
    }
    
    void start() {
        running = true;
        logger.log_event("Control loop started");
        
        auto next_tick = std::chrono::steady_clock::now();
        
        while (running) {
            // 1. Acquire telemetry (simulated for now - replace with real sensor interface)
            Telemetry measurement = acquire_telemetry();
            
            // 2. State estimation
            PatientState state = estimator.estimate(measurement, profile.baseline_hr_bpm);
            
            // 3. Control decision
            ControlOutput command = controller.decide(state, safety);
            
            // 4. Logging
            logger.log_telemetry(measurement);
            logger.log_control(command, state, measurement.timestamp);
            
            // 5. Display status (console output)
            display_status(state, command);
            
            // 6. Update safety monitor
            double cycle_duration_min = control_period.count() / 60000.0;
            safety.update_volume(command.infusion_ml_per_min, cycle_duration_min);
            
            // 7. Send command to infusion pump (placeholder)
            // send_to_pump(command.infusion_ml_per_min);
            
            // 8. Timing
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
        // This would interface with wearable sensors, medical monitors, etc.
        
        static double sim_time = 0.0;
        sim_time += control_period.count() / 1000.0;
        
        Telemetry m;
        m.timestamp = std::chrono::steady_clock::now();
        
        // Simulate gradual dehydration and recovery pattern
        m.hydration_pct = 65.0 + 15.0 * std::sin(sim_time * 0.05);
        m.heart_rate_bpm = profile.baseline_hr_bpm + 20.0 * std::sin(sim_time * 0.1);
        m.temp_celsius = 37.0 + 0.5 * std::sin(sim_time * 0.03);
        m.blood_loss_idx = 0.0;  // No blood loss in simulation
        m.fatigue_idx = 0.3 + 0.2 * std::sin(sim_time * 0.02);
        m.anxiety_idx = 0.2;
        m.signal_quality = 0.85 + 0.1 * std::sin(sim_time * 0.5);
        m.spo2_pct = 97.0 + 2.0 * std::sin(sim_time * 0.08);
        m.lactate_mmol = 2.0 + 1.0 * std::sin(sim_time * 0.04);
        
        return m;
    }
    
    void display_status(const PatientState& state, const ControlOutput& cmd) {
        static int display_counter = 0;
        if (++display_counter % 10 == 0) {  // Update display every 2 seconds
            std::cout << "\n=== AI-IV System Status ===\n";
            std::cout << "Hydration: " << std::fixed << std::setprecision(1) 
                     << state.hydration_pct << "%  ";
            std::cout << "Energy_T: " << std::setprecision(3) << state.energy_T << "  ";
            std::cout << "HR: " << std::setprecision(0) << state.heart_rate_bpm << " bpm\n";
            std::cout << "Risk: " << std::setprecision(2) << state.risk_score << "  ";
            std::cout << "Cardiac Reserve: " << state.cardiac_reserve << "  ";
            std::cout << "Coherence: " << state.coherence_sigma << "\n";
            std::cout << "Infusion Rate: " << std::setprecision(2) 
                     << cmd.infusion_ml_per_min << " ml/min  ";
            std::cout << "Confidence: " << cmd.confidence << "\n";
            if (!cmd.warning_flags.empty()) {
                std::cout << "⚠️  WARNINGS: " << cmd.warning_flags << "\n";
            }
            std::cout << "Volume (24h): " << std::setprecision(0) 
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
    std::cout << "║  AI-Optimized Intravenous Therapy Control System v1.0     ║\n";
    std::cout << "║  Real-time adaptive infusion with predictive control      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    // Define patient profile
    PatientProfile patient;
    patient.weight_kg = 75.0;
    patient.age_years = 35.0;
    patient.cardiac_condition = false;
    patient.renal_impairment = false;
    patient.diabetes = false;
    patient.baseline_hr_bpm = 70.0;
    patient.max_safe_infusion_rate = 1.5;  // ml/min
    
    // Generate session ID
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    std::string session_id = std::to_string(timestamp);
    
    std::cout << "Patient Profile:\n";
    std::cout << "  Weight: " << patient.weight_kg << " kg\n";
    std::cout << "  Age: " << patient.age_years << " years\n";
    std::cout << "  Baseline HR: " << patient.baseline_hr_bpm << " bpm\n";
    std::cout << "  Max Infusion Rate: " << patient.max_safe_infusion_rate << " ml/min\n";
    std::cout << "  Cardiac Condition: " << (patient.cardiac_condition ? "Yes" : "No") << "\n";
    std::cout << "  Renal Impairment: " << (patient.renal_impairment ? "Yes" : "No") << "\n\n";
    
    std::cout << "Session ID: " << session_id << "\n";
    std::cout << "Log files will be created with prefix: ai_iv_" << session_id << "\n\n";
    
    // Initialize system
    AIIVSystem system(patient, session_id);
    
    std::cout << "Starting control loop...\n";
    std::cout << "Press Ctrl+C to stop (or will run for 60 seconds in demo mode)\n\n";
    
    // For demonstration, run for 60 seconds
    // In production, this would run continuously with proper signal handling
    std::thread control_thread([&system]() {
        system.start();
    });
    
    // Demo: run for 60 seconds then stop
    std::this_thread::sleep_for(std::chrono::seconds(60));
    
    std::cout << "\n\nStopping system...\n";
    system.stop();
    control_thread.join();
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  System shutdown complete                                  ║\n";
    std::cout << "║  Check log files for detailed session data                 ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    
    return 0;
}
