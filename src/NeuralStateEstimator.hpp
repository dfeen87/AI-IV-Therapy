/*
 * NeuralStateEstimator.hpp
 *
 * Neural-network-based energy proxy estimator for the AI-IV system.
 *
 * Replaces StateEstimator::calculate_energy_proxy() with a 241-parameter
 * feedforward network (Dense-16-ReLU → Dense-8-ReLU → Dense-1-Sigmoid)
 * loaded at startup from a frugally-deep JSON model file.
 *
 * The model was trained via tools/train_sensor_fusion_model.py using
 * TensorFlow/Keras (knowledge distillation from the hand-crafted formula)
 * and exported in three formats:
 *
 *   models/sensor_fusion_fdeep.json   — loaded here by frugally-deep
 *   models/sensor_fusion.tflite       — TFLite flatbuffer (portable/embedded)
 *   models/sensor_fusion.h5           — Keras checkpoint
 *
 * Build flag:  -DENABLE_NEURAL_ESTIMATOR -I/usr/include/eigen3
 * Apt deps:    libfdeep-dev libeigen3-dev libfplus-dev nlohmann-json3-dev
 *
 * Interface is intentionally shaped after TFLite's Interpreter API so that
 * swapping in the official TFLite C runtime is a minimal change:
 *
 *   TfLiteInterpreter* → NeuralStateEstimator
 *   AllocateTensors()  → load()
 *   SetInputTensor()   → (passed directly to predict())
 *   Invoke()           → predict()
 *   GetOutputTensor()  → return value of predict()
 */

#pragma once

#ifdef ENABLE_NEURAL_ESTIMATOR

#include <fdeep/fdeep.hpp>
#include <optional>
#include <string>
#include <stdexcept>

namespace ivsys {

class NeuralStateEstimator {
public:
    // Load model from a frugally-deep JSON file.
    // Throws std::runtime_error if the file cannot be found or is invalid.
    void load(const std::string& model_path) {
        model_.emplace(fdeep::load_model(model_path,
            /*verify=*/true,
            fdeep::dev_null_logger));
        loaded_ = true;
    }

    bool is_loaded() const { return loaded_; }

    // Predict energy proxy E_T ∈ [0,1] from normalised telemetry inputs.
    // Inputs must already be normalised:
    //   hydration_norm  = hydration_pct  / 100.0
    //   hr_norm         = heart_rate_bpm / 200.0
    //   spo2_norm       = spo2_pct       / 100.0
    //   lactate_norm    = lactate_mmol   / 20.0
    //   fatigue         = fatigue_idx    (already [0,1])
    float predict(float hydration_norm,
                  float hr_norm,
                  float spo2_norm,
                  float lactate_norm,
                  float fatigue) const {
        if (!loaded_) {
            throw std::runtime_error(
                "NeuralStateEstimator::predict called before load()");
        }
        const auto result = model_->predict(
            {fdeep::tensor(fdeep::tensor_shape(5),
                std::vector<float>{
                    hydration_norm, hr_norm, spo2_norm, lactate_norm, fatigue
                })});
        return result[0].get(fdeep::tensor_pos(0));
    }

private:
    std::optional<fdeep::model> model_;
    bool loaded_ = false;
};

} // namespace ivsys

#endif // ENABLE_NEURAL_ESTIMATOR
