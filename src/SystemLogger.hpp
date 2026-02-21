#pragma once

#include "iv_system_types.hpp"
#include <string>
#include <fstream>
#include <optional>
#include <chrono>

namespace ivsys {

class SystemLogger {
private:
    std::ofstream log_file;
    std::ofstream telemetry_file;
    std::ofstream control_file;
    size_t log_flush_counter = 0;
    size_t telemetry_flush_counter = 0;
    size_t control_flush_counter = 0;
    static constexpr size_t kFlushEvery = 25;

    static std::string severity_to_string(AlertSeverity severity);
    static std::string escape_json_string(const std::string& input);
    void log_alert_event(const AlertEvent& event);

public:
    SystemLogger(const std::string& session_id);
    ~SystemLogger();

    void log_telemetry(const Telemetry& m);
    void log_control(const ControlOutput& out, const PatientState& state,
                     std::chrono::steady_clock::time_point t);
    void log_event(const std::string& event);
    void log_alert(AlertSeverity severity,
                   const std::string& source,
                   const std::string& code,
                   const std::string& message,
                   const std::optional<std::string>& context_json = std::nullopt);
};

} // namespace ivsys
