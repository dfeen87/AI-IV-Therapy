#pragma once

#include "../iv_system_types.hpp"

namespace ivsys {
namespace precision_spine {

// 64-byte aligned structure representing the unit of movement through the precision spine.
struct alignas(64) TreatmentFlow {
    PatientState state;
    double desired_rate;
    double confidence;
    bool is_valid;
    bool fallback_triggered;

    TreatmentFlow() : desired_rate(0.0), confidence(0.0), is_valid(true), fallback_triggered(false) {}

    explicit TreatmentFlow(const PatientState& s)
        : state(s), desired_rate(0.0), confidence(1.0 - s.uncertainty), is_valid(true), fallback_triggered(false) {}
};

} // namespace precision_spine
} // namespace ivsys
