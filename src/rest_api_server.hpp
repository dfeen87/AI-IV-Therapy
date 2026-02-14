/*
 * REST API Server for AI-IV Therapy Control System
 * Provides global network access to system telemetry, status, and control data
 * 
 * Design Principles:
 * - Non-blocking operation (runs in separate thread)
 * - Read-only access to control state (observability, not control)
 * - Lightweight HTTP/1.1 implementation
 * - JSON responses for modern client compatibility
 */

#ifndef REST_API_SERVER_HPP
#define REST_API_SERVER_HPP

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <vector>
#include <map>
#include <functional>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Forward declarations for data structures
struct Telemetry;
struct PatientState;

class RestApiServer {
public:
    RestApiServer(int port = 8080, const std::string& bind_address = "0.0.0.0");
    ~RestApiServer();
    
    // Server lifecycle
    bool start();
    void stop();
    bool is_running() const { return running_.load(); }
    
    // Data update methods (called from main control loop)
    void update_telemetry(const Telemetry& telemetry);
    void update_patient_state(const PatientState& state);
    void update_control_output(double infusion_rate, const std::string& rationale);
    void add_alert(const std::string& severity, const std::string& message);
    void update_config(const std::map<std::string, std::string>& config);
    
private:
    // Server configuration
    int port_;
    std::string bind_address_;
    int server_socket_;
    
    // Thread control
    std::atomic<bool> running_;
    std::thread server_thread_;
    
    // Data storage with thread-safe access
    mutable std::mutex data_mutex_;
    
    struct TelemetrySnapshot {
        double hydration_pct;
        double heart_rate_bpm;
        double temp_celsius;
        double spo2_pct;
        double lactate_mmol;
        double cardiac_output_L_min;
        std::string timestamp;
    };
    
    struct StateSnapshot {
        double hydration_pct;
        double energy_T;
        double metabolic_load;
        double cardiac_reserve;
        double risk_score;
    };
    
    struct ControlSnapshot {
        double infusion_rate;
        std::string rationale;
        std::string timestamp;
    };
    
    struct AlertRecord {
        std::string severity;
        std::string message;
        std::string timestamp;
    };
    
    TelemetrySnapshot current_telemetry_;
    StateSnapshot current_state_;
    ControlSnapshot current_control_;
    std::vector<AlertRecord> recent_alerts_;  // Keep last 100
    std::map<std::string, std::string> current_config_;
    
    // History buffers
    std::vector<TelemetrySnapshot> telemetry_history_;  // Keep last 1000
    
    // Server implementation
    void server_loop();
    void handle_client(int client_socket);
    
    // HTTP handling
    std::string parse_request_path(const std::string& request);
    std::string route_request(const std::string& method, const std::string& path);
    
    // API endpoints
    std::string handle_status();
    std::string handle_telemetry();
    std::string handle_telemetry_history();
    std::string handle_control();
    std::string handle_state();
    std::string handle_alerts();
    std::string handle_config();
    
    // HTTP response builders
    std::string build_http_response(int status_code, const std::string& body, 
                                   const std::string& content_type = "application/json");
    std::string build_json_error(const std::string& error);
    
    // Utility
    std::string get_current_timestamp();
    std::string escape_json_string(const std::string& str);
};

#endif // REST_API_SERVER_HPP
