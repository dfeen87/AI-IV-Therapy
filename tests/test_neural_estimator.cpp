#include "../src/NeuralStateEstimator.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

#ifndef NEURAL_MODEL_PATH
#error "Define NEURAL_MODEL_PATH at compile time, e.g. -DNEURAL_MODEL_PATH='\"models/sensor_fusion_fdeep.json\"'"
#endif

using namespace ivsys;

void test_load_and_healthy_patient() {
    NeuralStateEstimator est;
    est.load(NEURAL_MODEL_PATH);
    assert(est.is_loaded());

    // Healthy patient: hydration=80%, hr=75bpm, spo2=98%, lactate=2mmol, fatigue=0.3
    float et = est.predict(0.80f, 0.375f, 0.98f, 0.10f, 0.30f);
    std::cout << "  E_T (healthy):  " << et << " (expect > 0.6)\n";
    assert(et > 0.6f && et <= 1.0f);

    std::cout << "test_load_and_healthy_patient passed\n";
}

void test_stressed_patient() {
    NeuralStateEstimator est;
    est.load(NEURAL_MODEL_PATH);

    // Severely stressed: hydration=40%, hr=130bpm, spo2=84%, lactate=8mmol, fatigue=0.9
    float et = est.predict(0.40f, 0.65f, 0.84f, 0.40f, 0.90f);
    std::cout << "  E_T (stressed): " << et << " (expect < 0.5)\n";
    assert(et < 0.5f && et >= 0.0f);

    std::cout << "test_stressed_patient passed\n";
}

void test_rule_formula_agreement() {
    // The neural model should agree with the rule-based formula to within 0.08 MAE
    // across a range of inputs (spot-check — not exhaustive).
    // Threshold matches the CI training target (tools/train_sensor_fusion_model.py).
    NeuralStateEstimator est;
    est.load(NEURAL_MODEL_PATH);

    struct Sample { float h, hr, s, l, f; };
    const Sample cases[] = {
        {0.70f, 0.35f, 0.98f, 0.10f, 0.20f},
        {0.50f, 0.50f, 0.92f, 0.25f, 0.60f},
        {0.90f, 0.40f, 0.99f, 0.05f, 0.10f},
    };

    // Reference values from the Python formula (pre-computed)
    // hydration=70,hr=70,spo2=98,lac=2,fat=0.20 -> ~0.85
    // hydration=50,hr=100,spo2=92,lac=5,fat=0.60 -> ~0.55
    // hydration=90,hr=80,spo2=99,lac=1,fat=0.10 -> ~0.88
    const float refs[] = {0.85f, 0.55f, 0.88f};

    float total_err = 0.0f;
    for (int i = 0; i < 3; ++i) {
        float pred = est.predict(cases[i].h, cases[i].hr, cases[i].s, cases[i].l, cases[i].f);
        float err  = std::fabs(pred - refs[i]);
        total_err += err;
        std::cout << "  sample " << i << ": pred=" << pred
                  << " ref=" << refs[i] << " err=" << err << "\n";
    }
    float mae = total_err / 3.0f;
    std::cout << "  MAE across spot-check samples: " << mae << " (expect < 0.08)\n";
    assert(mae < 0.08f);

    std::cout << "test_rule_formula_agreement passed\n";
}

int main() {
    std::cout << "=== NeuralStateEstimator tests ===\n";
    test_load_and_healthy_patient();
    test_stressed_patient();
    test_rule_formula_agreement();
    std::cout << "All neural estimator tests passed\n";
    return 0;
}
