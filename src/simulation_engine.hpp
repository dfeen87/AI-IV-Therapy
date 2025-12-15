#pragma once

#include "iv_system_types.hpp"

namespace ivsys {

class SimulationEngine {
public:
    SimulationEngine();

    Telemetry generate_telemetry(double sim_time_seconds);

private:
    double base_hydration_;
    double base_hr_;
};

} // namespace ivsys
