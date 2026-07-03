#pragma once

#include "TreatmentFlow.hpp"
#include <vector>

namespace ivsys {
namespace precision_spine {

// Core routing primitives for the Precision-Flow Architecture
// Enforces deterministic constraint validation before adaptive dosing.

// deterministic routing of treatment signals
TreatmentFlow dose_route(const PatientState& raw_state);

// suppress low-confidence fluctuations
TreatmentFlow reject_noise(const TreatmentFlow& flow);

// enforce stability under noisy conditions
PatientState fallback_floor(const TreatmentFlow& flow);

// guarantee 64-byte alignment for all flows
// Allocates a vector of aligned TreatmentFlow objects
std::vector<TreatmentFlow> align_buffer(size_t size);

} // namespace precision_spine
} // namespace ivsys
