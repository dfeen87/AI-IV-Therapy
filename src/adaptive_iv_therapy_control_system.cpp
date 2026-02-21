/*
 * AI-Optimized Intravenous Therapy Control System
 * Enhanced with Full Energy Transfer Model from White Paper Section 4.1
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
#include <atomic>
#include <memory>
#include <map>

#include "iv_system_types.hpp"
#include "config_defaults.hpp"
#include "Utils.hpp"
#include "SystemLogger.hpp"
#include "SafetyMonitor.hpp"
#include "StateEstimator.hpp"
#include "AdaptiveController.hpp"

// REST API Server (optional - enable with -DENABLE_REST_API flag)
#ifdef ENABLE_REST_API
#define TELEMETRY_DEFINED
#define PATIENT_STATE_DEFINED
#include "rest_api_server.hpp"
#endif

using namespace ivsys;

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
    
    std::atomic<bool> running;
    double current_infusion_rate;
    const std::chrono::milliseconds control_period{200};  // 5 Hz
    
#ifdef ENABLE_REST_API
    std::unique_ptr<RestApiServer> rest_api;
#endif
    
public:
    AIIVSystem(const PatientProfile& prof, const std::string& session_id)
        : profile(prof), controller(prof), safety(prof), logger(session_id), 
          running(false), current_infusion_rate(0.4) {
        logger.log_event("System initialized - Enhanced Energy Transfer Model v1.0");
        logger.log_event("Patient: " + std::to_string(prof.weight_kg) + "kg, " + 
                        std::to_string(prof.age_years) + "y");
        logger.log_event("Optimal flow velocity: " + 
                        std::to_string(prof.energy_params.v_optimal_cm_s) + " cm/s");
        
#ifdef ENABLE_REST_API
        // Initialize REST API server on port 8080
        rest_api = std::make_unique<RestApiServer>(8080, "0.0.0.0");
        
        // Set initial configuration
        std::map<std::string, std::string> config;
        config["patient_weight_kg"] = std::to_string(prof.weight_kg);
        config["patient_age_years"] = std::to_string(prof.age_years);
        config["max_infusion_rate"] = std::to_string(prof.max_safe_infusion_rate);
        config["baseline_hr_bpm"] = std::to_string(prof.baseline_hr_bpm);
        config["session_id"] = session_id;
        rest_api->update_config(config);
        
        logger.log_event("REST API initialized on port 8080");
#endif
    }
    
    void start() {
        running = true;
        logger.log_event("Control loop started");
        
#ifdef ENABLE_REST_API
        if (rest_api && rest_api->start()) {
            logger.log_event("REST API server started on port 8080");
        }
#endif
        
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

#ifdef ENABLE_REST_API
            // Update REST API with current data
            if (rest_api) {
                rest_api->update_telemetry(measurement);
                rest_api->update_patient_state(state);
                rest_api->update_control_output(command.infusion_ml_per_min, command.rationale);
            }
#endif

            if (measurement.signal_quality < 0.6) {
                logger.log_alert(
                    AlertSeverity::Warn,
                    "Telemetry",
                    "SENSOR_QUALITY_LOW",
                    "Telemetry signal quality below threshold",
                    std::string("{\"signal_quality\":") +
                        std::to_string(measurement.signal_quality) +
                        ",\"threshold\":0.6}");
#ifdef ENABLE_REST_API
                if (rest_api) {
                    rest_api->add_alert("warning", "Telemetry signal quality below threshold");
                }
#endif
            }

            if (!command.warning_flags.empty()) {
                if (has_warning_flag(command.warning_flags, "VOLUME_LIMIT_APPROACH")) {
                    logger.log_alert(
                        AlertSeverity::Warn,
                        "SafetyMonitor",
                        "VOLUME_LIMIT_APPROACH",
                        "Projected volume approaching 24h limit",
                        std::string("{\"cumulative_volume_ml\":") +
                            std::to_string(safety.get_cumulative_volume()) + "}");
                }
                if (has_warning_flag(command.warning_flags, "LOW_CARDIAC_RESERVE")) {
                    logger.log_alert(
                        AlertSeverity::Warn,
                        "SafetyMonitor",
                        "LOW_CARDIAC_RESERVE",
                        "Cardiac reserve below minimum threshold",
                        std::string("{\"cardiac_reserve\":") +
                            std::to_string(state.cardiac_reserve) + "}");
                }
                if (has_warning_flag(command.warning_flags, "RATE_CHANGE_LIMITED")) {
                    logger.log_alert(
                        AlertSeverity::Info,
                        "SafetyMonitor",
                        "RATE_CHANGE_LIMITED",
                        "Infusion rate change limited by safety constraints",
                        std::string("{\"infusion_rate_ml_min\":") +
                            std::to_string(command.infusion_ml_per_min) + "}");
                }
                if (has_warning_flag(command.warning_flags, "HIGH_RISK_STATE")) {
                    logger.log_alert(
                        AlertSeverity::Warn,
                        "SafetyMonitor",
                        "HIGH_RISK_STATE",
                        "Risk score exceeded threshold",
                        std::string("{\"risk_score\":") +
                            std::to_string(state.risk_score) + "}");
                }
                if (has_warning_flag(command.warning_flags, "TACHYCARDIA_DETECTED")) {
                    logger.log_alert(
                        AlertSeverity::Warn,
                        "SafetyMonitor",
                        "TACHYCARDIA_DETECTED",
                        "Tachycardia detected",
                        std::string("{\"heart_rate_bpm\":") +
                            std::to_string(state.heart_rate_bpm) + "}");
                }
                if (has_warning_flag(command.warning_flags, "EMERGENCY_MIN_RATE")) {
                    logger.log_alert(
                        AlertSeverity::Critical,
                        "SafetyMonitor",
                        "EMERGENCY_MIN_RATE",
                        "Emergency minimum infusion rate enforced",
                        std::string("{\"hydration_pct\":") +
                            std::to_string(state.hydration_pct) + "}");
                }
            }
            
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
        
#ifdef ENABLE_REST_API
        if (rest_api) {
            rest_api->stop();
            logger.log_event("REST API server stopped");
        }
#endif
    }
    
private:
    static bool has_warning_flag(const std::string& flags, const std::string& token) {
        return flags.find(token) != std::string::npos;
    }

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

#ifndef AI_IV_ALERT_LOG_TEST
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
    
    // Validate patient parameters
    if (patient.weight_kg <= 0.0) {
        std::cerr << "Error: Patient weight must be positive (got " 
                  << patient.weight_kg << " kg)\n";
        return 1;
    }
    if (patient.age_years <= 0.0) {
        std::cerr << "Error: Patient age must be positive (got " 
                  << patient.age_years << " years)\n";
        return 1;
    }
    
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
#else
int main() {
    const std::string session_id = "alert_test";
    {
        SystemLogger logger(session_id);
        logger.log_alert(
            AlertSeverity::Critical,
            "AlertTest",
            "ALERT_EMIT_TEST",
            "Alert emission smoke test",
            std::string("{\"sample\":true}"));
    }

    std::ifstream log_stream("ai_iv_" + session_id + "_system.log");
    std::string line;
    if (!std::getline(log_stream, line)) {
        return 1;
    }
    if (line.rfind("ALERT ", 0) != 0) {
        return 2;
    }
    if (line.find("\"severity\"") == std::string::npos ||
        line.find("\"source\"") == std::string::npos ||
        line.find("\"code\"") == std::string::npos ||
        line.find("\"message\"") == std::string::npos) {
        return 3;
    }
    return 0;
}
#endif
