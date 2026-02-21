#include "../src/StateEstimator.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace ivsys;

void test_estimate_basic() {
    PatientProfile profile;
    profile.weight_kg = 70.0;
    profile.baseline_hr_bpm = 70.0;

    StateEstimator estimator;

    Telemetry m;
    m.hydration_pct = 80.0;
    m.heart_rate_bpm = 75.0;
    m.temp_celsius = 37.0;
    m.spo2_pct = 98.0;
    m.signal_quality = 1.0;

    PatientState state = estimator.estimate(m, profile, 1.0);

    if (std::abs(state.hydration_pct - 80.0) > 0.1) {
        std::cerr << "test_estimate_basic failed: hydration_pct mismatch\n";
        exit(1);
    }
    if (state.heart_rate_bpm != 75.0) {
        std::cerr << "test_estimate_basic failed: heart_rate_bpm mismatch\n";
        exit(1);
    }
    if (state.uncertainty > 0.5) {
        std::cerr << "test_estimate_basic failed: uncertainty too high\n";
        exit(1);
    }

    std::cout << "test_estimate_basic passed\n";
}

int main() {
    test_estimate_basic();
    return 0;
}
