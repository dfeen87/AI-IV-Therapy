// Copyright (c) Don Michael Feeney Jr.
// Licensed under the MIT License.

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

void test_invalid_weight_remains_finite() {
    PatientProfile profile;
    profile.weight_kg = 0.0;

    Telemetry m;
    m.hydration_pct = 80.0;
    m.heart_rate_bpm = 75.0;
    m.temp_celsius = 37.0;
    m.spo2_pct = 98.0;
    m.signal_quality = 1.0;

    StateEstimator estimator;
    PatientState state = estimator.estimate(m, profile, -1.0);
    if (!std::isfinite(state.energy_T_absolute)) {
        std::cerr << "test_invalid_weight_remains_finite failed: non-finite energy transfer\n";
        exit(1);
    }

    std::cout << "test_invalid_weight_remains_finite passed\n";
}

void test_prediction_uses_sample_intervals() {
    PatientProfile profile;
    profile.weight_kg = 70.0;

    StateEstimator estimator;
    Telemetry m;
    m.heart_rate_bpm = 75.0;
    m.temp_celsius = 37.0;
    m.spo2_pct = 98.0;
    m.signal_quality = 1.0;

    for (double hydration : {50.0, 60.0, 70.0, 80.0, 90.0}) {
        m.hydration_pct = hydration;
        estimator.estimate(m, profile, 1.0);
    }

    auto prediction = estimator.predict_forward(1);
    if (!prediction || std::abs(prediction->hydration_pct - 100.0) > 0.1) {
        std::cerr << "test_prediction_uses_sample_intervals failed: incorrect trend\n";
        exit(1);
    }
    if (estimator.predict_forward(-1).has_value()) {
        std::cerr << "test_prediction_uses_sample_intervals failed: accepted negative horizon\n";
        exit(1);
    }

    std::cout << "test_prediction_uses_sample_intervals passed\n";
}

int main() {
    test_estimate_basic();
    test_invalid_weight_remains_finite();
    test_prediction_uses_sample_intervals();
    return 0;
}
