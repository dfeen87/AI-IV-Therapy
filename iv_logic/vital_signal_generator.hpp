#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../src/iv_system_types.hpp"

namespace ivsys {
namespace extensions {

// AILEE-style ModelSignal representation
struct ModelSignal {
    double value = 0.0;
    double confidence = 0.0;
    std::string model_id;
    std::string degradation_reason; // Documented rules for confidence degradation
};

// Not thread-safe: instances must be accessed from a single thread.
// Each VitalSignalGenerator maintains mutable previous-value state for
// stability analysis; guard access externally if shared across threads.
class VitalSignalGenerator {
public:
    VitalSignalGenerator();

    // Convert simulated patient vitals into AILEE-style ModelSignal objects.
    std::vector<ModelSignal> generate_signals(const ivsys::Telemetry& telemetry);

private:
    double calculate_confidence(double raw_confidence, double stability_penalty, std::string& reason);

    // Track previous values for stability analysis
    double prev_hr_ = 0.0;
    double prev_bp_sys_ = 0.0; // Simulated BP based on HR/other factors
    double prev_spo2_ = 0.0;
    double prev_temp_ = 0.0;

    // Simple state
    bool first_run_ = true;
};

} // namespace extensions
} // namespace ivsys
