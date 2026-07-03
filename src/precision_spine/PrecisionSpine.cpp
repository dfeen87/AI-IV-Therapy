#include "PrecisionSpine.hpp"
#include "../Utils.hpp"
#include <algorithm>

namespace ivsys {
namespace precision_spine {

TreatmentFlow dose_route(const PatientState& raw_state) {
    // Create the basic flow structure, already aligned
    TreatmentFlow flow(raw_state);

    // Baseline validation routing
    if (flow.state.hydration_pct < 0.0 || flow.state.hydration_pct > 100.0) {
        flow.is_valid = false;
        flow.state.hydration_pct = Utils::clamp(flow.state.hydration_pct, 0.0, 100.0);
    }

    return flow;
}

TreatmentFlow reject_noise(const TreatmentFlow& flow) {
    TreatmentFlow clean_flow = flow;

    // If uncertainty is too high (confidence too low), suppress the state
    if (clean_flow.state.uncertainty > 0.8) {
        clean_flow.is_valid = false;
        // Dampen potentially noisy metrics
        clean_flow.state.coherence_sigma *= 0.5;
        clean_flow.state.risk_score = std::max(clean_flow.state.risk_score, 0.5); // Assume higher risk if uncertain
    }

    // Filter impossible physiological states (noise artifacts)
    if (clean_flow.state.heart_rate_bpm < 20.0 || clean_flow.state.heart_rate_bpm > 300.0) {
         clean_flow.is_valid = false;
    }

    return clean_flow;
}

PatientState fallback_floor(const TreatmentFlow& flow) {
    PatientState safe_state = flow.state;

    // Enforce the fallback floor: if the flow is invalid or highly unstable,
    // force physiological variables to a conservative baseline
    if (!flow.is_valid || flow.state.coherence_sigma < 0.2) {
        safe_state.coherence_sigma = 0.2; // Floor on coherence

        // Cap energy delivery and risk assumptions during a fallback
        safe_state.energy_T = std::min(safe_state.energy_T, 0.5);
        safe_state.risk_score = std::max(safe_state.risk_score, 0.7); // Assume higher risk
        safe_state.cardiac_reserve = std::max(safe_state.cardiac_reserve, 0.1); // Avoid dividing by zero or going negative
    }

    // Always enforce strict bounds on flow velocity and efficiency
    safe_state.estimated_flow_velocity_cm_s = std::max(safe_state.estimated_flow_velocity_cm_s, 0.0);
    safe_state.flow_efficiency = Utils::clamp(safe_state.flow_efficiency, 0.0, 1.0);

    return safe_state;
}

std::vector<TreatmentFlow> align_buffer(size_t size) {
    // std::vector will correctly allocate memory observing the alignas(64) of TreatmentFlow
    // up to the system's max alignment, and with C++17 new over-aligned allocations are supported.
    // To strictly enforce 64-byte alignment, we rely on the alignas(64) specification in TreatmentFlow.hpp.
    std::vector<TreatmentFlow> buffer(size);
    return buffer;
}

} // namespace precision_spine
} // namespace ivsys
