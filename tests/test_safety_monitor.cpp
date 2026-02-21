#include "../src/SafetyMonitor.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace ivsys;

void test_volume_limit() {
    PatientProfile profile;
    profile.weight_kg = 70.0;
    profile.cardiac_condition = false;
    profile.renal_impairment = false;
    profile.max_safe_infusion_rate = 1.5;

    SafetyMonitor monitor(profile);

    // Simulate filling up the volume
    // Max volume = 70 * 35 = 2450 ml
    // SafetyMonitor::update_volume adds to cumulative_volume_ml
    // Add enough to trigger warning (>90% of limit)
    monitor.update_volume(100.0, 23.0); // 2300 ml > 2205

    PatientState state;
    state.cardiac_reserve = 1.0;
    state.risk_score = 0.0;
    state.heart_rate_bpm = 70.0;
    state.hydration_pct = 90.0;

    // Pass 1.0 minute as dt
    auto check = monitor.evaluate(1.0, state, 1.0);

    // Should be limited because 2300 is > 0.9 * 2450 (2205)
    if (check.max_allowed_rate > 0.3) {
        std::cerr << "test_volume_limit failed: max_allowed_rate " << check.max_allowed_rate << " > 0.3\n";
        exit(1);
    }
    if (check.warnings.find("VOLUME_LIMIT_APPROACH") == std::string::npos) {
         std::cerr << "test_volume_limit failed: warning missing\n";
         exit(1);
    }

    std::cout << "test_volume_limit passed\n";
}

void test_cardiac_reserve() {
    PatientProfile profile;
    profile.weight_kg = 70.0;
    profile.cardiac_condition = false;
    profile.max_safe_infusion_rate = 1.5;

    SafetyMonitor monitor(profile);

    PatientState state;
    state.cardiac_reserve = 0.1; // Very low
    state.risk_score = 0.0;
    state.heart_rate_bpm = 70.0;
    state.hydration_pct = 90.0;

    // Pass 1.0 minute as dt
    auto check = monitor.evaluate(1.0, state, 1.0);

    if (check.max_allowed_rate > 0.5) {
        std::cerr << "test_cardiac_reserve failed: max_allowed_rate " << check.max_allowed_rate << " > 0.5\n";
        exit(1);
    }
    if (check.warnings.find("LOW_CARDIAC_RESERVE") == std::string::npos) {
         std::cerr << "test_cardiac_reserve failed: warning missing\n";
         exit(1);
    }

    std::cout << "test_cardiac_reserve passed\n";
}

int main() {
    test_volume_limit();
    test_cardiac_reserve();
    return 0;
}
