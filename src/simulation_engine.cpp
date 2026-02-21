#include "simulation_engine.hpp"
#include <cmath>

namespace ivsys {

SimulationEngine::SimulationEngine()
    : base_hydration_(75.0),
      base_hr_(70.0) {}

Telemetry SimulationEngine::generate_telemetry(double t) {
    Telemetry m{};
    m.hydration_pct = base_hydration_ + 10.0 * std::sin(t * 0.05);
    m.heart_rate_bpm = base_hr_ + 15.0 * std::sin(t * 0.1);
    m.spo2_pct = 97.0;
    m.temp_celsius = 37.0;
    m.lactate_mmol = 2.0;
    m.fatigue_idx = 0.3;
    m.blood_loss_idx = 0.0;
    return m;
}

} // namespace ivsys
