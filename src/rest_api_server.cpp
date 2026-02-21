/*
 * REST API Server Implementation
 */

#include "rest_api_server.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iostream>

RestApiServer::RestApiServer(int port, const std::string& bind_address)
    : port_(port), bind_address_(bind_address), server_socket_(-1), running_(false) {
    
    // Initialize with default values
    current_telemetry_ = {0, 0, 0, 0, 0, 0, ""};
    current_state_ = {0, 0, 0, 0, 0};
    current_control_ = {0, "", ""};
}

RestApiServer::~RestApiServer() {
    stop();
}

bool RestApiServer::start() {
    if (running_.load()) {
        return false;  // Already running
    }
    
    // Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        std::cerr << "Failed to create server socket" << std::endl;
        return false;
    }
    
    // Set socket options to allow reuse
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(server_socket_);
        return false;
    }
    
    // Bind to address
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port_);
    
    if (bind_address_ == "0.0.0.0") {
        address.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, bind_address_.c_str(), &address.sin_addr) <= 0) {
            std::cerr << "Invalid bind address" << std::endl;
            close(server_socket_);
            return false;
        }
    }
    
    if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port_ << std::endl;
        close(server_socket_);
        return false;
    }
    
    // Start listening
    if (listen(server_socket_, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_socket_);
        return false;
    }
    
    // Start server thread
    running_.store(true);
    server_thread_ = std::thread(&RestApiServer::server_loop, this);
    
    std::cout << "REST API Server started on " << bind_address_ << ":" << port_ << std::endl;
    return true;
}

void RestApiServer::stop() {
    if (running_.load()) {
        running_.store(false);
        
        // Close server socket to unblock accept()
        if (server_socket_ >= 0) {
            close(server_socket_);
            server_socket_ = -1;
        }
        
        // Wait for thread to finish
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        
        std::cout << "REST API Server stopped" << std::endl;
    }
}

void RestApiServer::server_loop() {
    while (running_.load()) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            if (running_.load()) {
                std::cerr << "Accept failed" << std::endl;
            }
            break;
        }
        
        // Handle client in same thread (simple implementation)
        // Could be improved with thread pool for production
        handle_client(client_socket);
        close(client_socket);
    }
}

void RestApiServer::handle_client(int client_socket) {
    char buffer[4096];
    ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        return;
    }
    
    buffer[bytes_read] = '\0';
    std::string request(buffer);
    
    // Parse HTTP method and path
    std::istringstream request_stream(request);
    std::string method, path, version;
    request_stream >> method >> path >> version;
    
    // Route request
    std::string response = route_request(method, path);
    
    // Send response
    send(client_socket, response.c_str(), response.length(), 0);
}

std::string RestApiServer::route_request(const std::string& method, const std::string& path) {
    // Only support GET for safety (read-only API)
    if (method != "GET") {
        return build_http_response(405, build_json_error("Method not allowed"));
    }
    
    // Route to appropriate handler
    if (path == "/api/status" || path == "/api/status/") {
        return build_http_response(200, handle_status());
    } else if (path == "/api/telemetry" || path == "/api/telemetry/") {
        return build_http_response(200, handle_telemetry());
    } else if (path == "/api/telemetry/history" || path == "/api/telemetry/history/") {
        return build_http_response(200, handle_telemetry_history());
    } else if (path == "/api/control" || path == "/api/control/") {
        return build_http_response(200, handle_control());
    } else if (path == "/api/state" || path == "/api/state/") {
        return build_http_response(200, handle_state());
    } else if (path == "/api/alerts" || path == "/api/alerts/") {
        return build_http_response(200, handle_alerts());
    } else if (path == "/api/config" || path == "/api/config/") {
        return build_http_response(200, handle_config());
    } else if (path == "/" || path == "/api" || path == "/api/") {
        // Root endpoint - list available endpoints
        std::ostringstream json;
        json << "{"
             << "\"service\":\"AI-IV Therapy REST API\","
             << "\"version\":\"4.0.0\","
             << "\"endpoints\":["
             << "\"/api/status\","
             << "\"/api/telemetry\","
             << "\"/api/telemetry/history\","
             << "\"/api/control\","
             << "\"/api/state\","
             << "\"/api/alerts\","
             << "\"/api/config\""
             << "]"
             << "}";
        return build_http_response(200, json.str());
    } else {
        return build_http_response(404, build_json_error("Endpoint not found"));
    }
}

std::string RestApiServer::handle_status() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    std::ostringstream json;
    json << "{"
         << "\"status\":\"running\","
         << "\"timestamp\":\"" << get_current_timestamp() << "\","
         << "\"api_version\":\"4.0.0\","
         << "\"system\":\"AI-IV Therapy Control System\""
         << "}";
    
    return json.str();
}

std::string RestApiServer::handle_telemetry() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    json << "{"
         << "\"timestamp\":\"" << current_telemetry_.timestamp << "\","
         << "\"hydration_pct\":" << current_telemetry_.hydration_pct << ","
         << "\"heart_rate_bpm\":" << current_telemetry_.heart_rate_bpm << ","
         << "\"temp_celsius\":" << current_telemetry_.temp_celsius << ","
         << "\"spo2_pct\":" << current_telemetry_.spo2_pct << ","
         << "\"lactate_mmol\":" << current_telemetry_.lactate_mmol << ","
         << "\"cardiac_output_L_min\":" << current_telemetry_.cardiac_output_L_min
         << "}";
    
    return json.str();
}

std::string RestApiServer::handle_telemetry_history() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    json << "{\"history\":[";
    
    for (size_t i = 0; i < telemetry_history_.size(); ++i) {
        if (i > 0) json << ",";
        const auto& t = telemetry_history_[i];
        json << "{"
             << "\"timestamp\":\"" << t.timestamp << "\","
             << "\"hydration_pct\":" << t.hydration_pct << ","
             << "\"heart_rate_bpm\":" << t.heart_rate_bpm << ","
             << "\"temp_celsius\":" << t.temp_celsius << ","
             << "\"spo2_pct\":" << t.spo2_pct << ","
             << "\"lactate_mmol\":" << t.lactate_mmol << ","
             << "\"cardiac_output_L_min\":" << t.cardiac_output_L_min
             << "}";
    }
    
    json << "],\"count\":" << telemetry_history_.size() << "}";
    return json.str();
}

std::string RestApiServer::handle_control() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    std::ostringstream json;
    json << std::fixed << std::setprecision(3);
    json << "{"
         << "\"timestamp\":\"" << current_control_.timestamp << "\","
         << "\"infusion_rate_ml_min\":" << current_control_.infusion_rate << ","
         << "\"rationale\":\"" << escape_json_string(current_control_.rationale) << "\""
         << "}";
    
    return json.str();
}

std::string RestApiServer::handle_state() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    std::ostringstream json;
    json << std::fixed << std::setprecision(3);
    json << "{"
         << "\"hydration_pct\":" << current_state_.hydration_pct << ","
         << "\"energy_T\":" << current_state_.energy_T << ","
         << "\"metabolic_load\":" << current_state_.metabolic_load << ","
         << "\"cardiac_reserve\":" << current_state_.cardiac_reserve << ","
         << "\"risk_score\":" << current_state_.risk_score
         << "}";
    
    return json.str();
}

std::string RestApiServer::handle_alerts() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    std::ostringstream json;
    json << "{\"alerts\":[";
    
    for (size_t i = 0; i < recent_alerts_.size(); ++i) {
        if (i > 0) json << ",";
        const auto& alert = recent_alerts_[i];
        json << "{"
             << "\"timestamp\":\"" << alert.timestamp << "\","
             << "\"severity\":\"" << alert.severity << "\","
             << "\"message\":\"" << escape_json_string(alert.message) << "\""
             << "}";
    }
    
    json << "],\"count\":" << recent_alerts_.size() << "}";
    return json.str();
}

std::string RestApiServer::handle_config() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    std::ostringstream json;
    json << "{\"config\":{";
    
    bool first = true;
    for (const auto& pair : current_config_) {
        if (!first) json << ",";
        json << "\"" << escape_json_string(pair.first) << "\":"
             << "\"" << escape_json_string(pair.second) << "\"";
        first = false;
    }
    
    json << "}}";
    return json.str();
}

void RestApiServer::update_telemetry(const ivsys::Telemetry& telemetry) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    TelemetrySnapshot snapshot;
    snapshot.hydration_pct = telemetry.hydration_pct;
    snapshot.heart_rate_bpm = telemetry.heart_rate_bpm;
    snapshot.temp_celsius = telemetry.temp_celsius;
    snapshot.spo2_pct = telemetry.spo2_pct;
    snapshot.lactate_mmol = telemetry.lactate_mmol;
    snapshot.cardiac_output_L_min = telemetry.cardiac_output_L_min;
    snapshot.timestamp = get_current_timestamp();
    
    current_telemetry_ = snapshot;
    
    // Add to history, keep last 1000
    telemetry_history_.push_back(snapshot);
    if (telemetry_history_.size() > 1000) {
        telemetry_history_.erase(telemetry_history_.begin());
    }
}

void RestApiServer::update_patient_state(const ivsys::PatientState& state) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    current_state_.hydration_pct = state.hydration_pct;
    current_state_.energy_T = state.energy_T;
    current_state_.metabolic_load = state.metabolic_load;
    current_state_.cardiac_reserve = state.cardiac_reserve;
    current_state_.risk_score = state.risk_score;
}

void RestApiServer::update_control_output(double infusion_rate, const std::string& rationale) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    current_control_.infusion_rate = infusion_rate;
    current_control_.rationale = rationale;
    current_control_.timestamp = get_current_timestamp();
}

void RestApiServer::add_alert(const std::string& severity, const std::string& message) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    AlertRecord alert;
    alert.severity = severity;
    alert.message = message;
    alert.timestamp = get_current_timestamp();
    
    recent_alerts_.push_back(alert);
    
    // Keep only last 100 alerts
    if (recent_alerts_.size() > 100) {
        recent_alerts_.erase(recent_alerts_.begin());
    }
}

void RestApiServer::update_config(const std::map<std::string, std::string>& config) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    current_config_ = config;
}

std::string RestApiServer::build_http_response(int status_code, const std::string& body,
                                               const std::string& content_type) {
    std::ostringstream response;
    
    std::string status_text;
    switch (status_code) {
        case 200: status_text = "OK"; break;
        case 404: status_text = "Not Found"; break;
        case 405: status_text = "Method Not Allowed"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }
    
    response << "HTTP/1.1 " << status_code << " " << status_text << "\r\n"
             << "Content-Type: " << content_type << "\r\n"
             << "Content-Length: " << body.length() << "\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body;
    
    return response.str();
}

std::string RestApiServer::build_json_error(const std::string& error) {
    std::ostringstream json;
    json << "{\"error\":\"" << escape_json_string(error) << "\"}";
    return json.str();
}

std::string RestApiServer::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    
    return oss.str();
}

std::string RestApiServer::escape_json_string(const std::string& str) {
    std::ostringstream escaped;
    for (char c : str) {
        switch (c) {
            case '"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default: escaped << c; break;
        }
    }
    return escaped.str();
}
