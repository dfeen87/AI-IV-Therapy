#include "SystemLogger.hpp"
#include "Utils.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace ivsys {

std::string SystemLogger::severity_to_string(AlertSeverity severity) {
    switch (severity) {
        case AlertSeverity::Debug: return "DEBUG";
        case AlertSeverity::Info: return "INFO";
        case AlertSeverity::Warn: return "WARN";
        case AlertSeverity::Error: return "ERROR";
        case AlertSeverity::Critical: return "CRITICAL";
    }
    return "INFO";
}

std::string SystemLogger::escape_json_string(const std::string& input) {
    std::ostringstream oss;
    for (char c : input) {
        switch (c) {
            case '\\': oss << "\\\\"; break;
            case '"': oss << "\\\""; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    oss << "\\u"
                        << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(c))
                        << std::dec << std::setfill(' ');
                } else {
                    oss << c;
                }
        }
    }
    return oss.str();
}

void SystemLogger::log_alert_event(const AlertEvent& event) {
    log_file << "ALERT {\"timestamp\":" << event.timestamp_ms
             << ",\"severity\":\"" << severity_to_string(event.severity) << "\""
             << ",\"source\":\"" << escape_json_string(event.source) << "\""
             << ",\"code\":\"" << escape_json_string(event.code) << "\""
             << ",\"message\":\"" << escape_json_string(event.message) << "\"";
    if (event.context_json && !event.context_json->empty()) {
        log_file << ",\"context\":" << *event.context_json;
    }
    log_file << "}\n";

    if (++log_flush_counter % kFlushEvery == 0) {
        log_file.flush();
    }
    if (event.severity == AlertSeverity::Critical) {
        log_file.flush();
    }
}

SystemLogger::SystemLogger(const std::string& session_id) {
    std::string prefix = "ai_iv_" + session_id;
    log_file.open(prefix + "_system.log");
    telemetry_file.open(prefix + "_telemetry.csv");
    control_file.open(prefix + "_control.csv");
    if (!log_file.is_open() || !telemetry_file.is_open() || !control_file.is_open()) {
        throw std::runtime_error("Failed to open one or more log files for session " + session_id);
    }

    telemetry_file << "timestamp,hydration_pct,heart_rate_bpm,temp_c,blood_loss_idx,"
                   << "fatigue_idx,anxiety_idx,signal_quality,spo2_pct,lactate_mmol,"
                   << "cardiac_output_L_min\n";

    control_file << "timestamp,infusion_rate_ml_min,confidence,energy_T,energy_T_abs_W_kg,"
                << "flow_velocity_cm_s,flow_efficiency,risk_score,"
                << "cardiac_reserve,warnings,rationale\n";
}

SystemLogger::~SystemLogger() {
    log_file.close();
    telemetry_file.close();
    control_file.close();
}

void SystemLogger::log_telemetry(const Telemetry& m) {
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
    if (++telemetry_flush_counter % kFlushEvery == 0) {
        telemetry_file.flush();
    }
}

void SystemLogger::log_control(const ControlOutput& out, const PatientState& state,
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
    if (++control_flush_counter % kFlushEvery == 0) {
        control_file.flush();
    }
}

void SystemLogger::log_event(const std::string& event) {
    auto now = std::chrono::steady_clock::now();
    log_file << "[" << Utils::timestamp_str(now) << "] " << event << "\n";
    if (++log_flush_counter % kFlushEvery == 0) {
        log_file.flush();
    }
}

void SystemLogger::log_alert(AlertSeverity severity,
               const std::string& source,
               const std::string& code,
               const std::string& message,
               const std::optional<std::string>& context_json) {
    AlertEvent event{
        Utils::epoch_ms(),
        severity,
        source,
        code,
        message,
        context_json
    };
    log_alert_event(event);
}

} // namespace ivsys
