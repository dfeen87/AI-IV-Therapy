#pragma once

#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace ai_iv {
namespace domains {
namespace metabojoint {

/**
 * @brief Pre-clinical deterministic state estimator and safety monitor 
 * for the MetaboJoint V3.1 in vivo delivery substrate.
 * * Safety Is Structural: Payload release is strictly mathematically entombed
 * until the cooperative mesh expansion breaches the cargo's hydrodynamic radius.
 */
class MetaboJointVault {
private:
    // --- V3.1 Immutable Physical Constants ---
    static constexpr double HYDRODYNAMIC_RADIUS_NM = 9.0; // Cas9 RNP R_H
    static constexpr double OVERLAP_PARAM_Y = 1.0;        // CBMA Critical overlap
    static constexpr double VOL_FRACTION_V2S = 0.18;      // Equilibrium swollen vol fraction
    
    // --- Bounded State Variables ---
    double current_mesh_size_nm = 2.0;    // Sealed state at pH 7.4
    double cleavage_progression_pct = 0.0;
    double current_payload_pct = 100.0;

public:
    MetaboJointVault() = default;

    /**
     * @brief Updates the biochemical telemetry and recalculates the structural state.
     * @param ambient_ph The local pH environment (telemetry)
     * @param cathepsin_k_activity Normalized enzyme activity 0.0 to 1.0 (telemetry)
     */
    void update_telemetry(double ambient_ph, double cathepsin_k_activity) {
        // Enforce safety bounds on telemetry inputs
        ambient_ph = std::clamp(ambient_ph, 1.0, 8.0);
        cathepsin_k_activity = std::clamp(cathepsin_k_activity, 0.0, 1.0);

        // State Estimation: Calculate cleavage progression (simplified proxy model)
        // In the marrow niche (pH ~6.5 - 7.1), Cathepsin K becomes highly active
        if (ambient_ph <= 7.1 && cathepsin_k_activity > 0.1) {
             // Incremental cleavage modeled per tick
             cleavage_progression_pct = std::clamp(cleavage_progression_pct + (cathepsin_k_activity * 2.0), 0.0, 100.0);
        }

        // State Estimation: Peppas-Rehner Mesh Expansion
        // Expands from ~2.0nm up to ~12.0nm as crosslinks are cleaved
        current_mesh_size_nm = 2.0 + (10.0 * (cleavage_progression_pct / 100.0));
    }

    /**
     * @brief Calculates the deterministic Fickian diffusion rate.
     * Incorporates the structural safety constraint (Steric Trap).
     */
    [[nodiscard]] double calculate_fickian_efficiency() const {
        // SAFETY MONITOR: The Steric Trap (Hard Constraint)
        // If the mesh is smaller than the payload, diffusion is strictly 0.0
        if (current_mesh_size_nm <= HYDRODYNAMIC_RADIUS_NM) {
            return 0.0; 
        }

        // CONTROLLER: Lustig-Peppas Fickian Diffusion Model
        double fractional_escape = 1.0 - (HYDRODYNAMIC_RADIUS_NM / current_mesh_size_nm);
        double hydration_penalty = std::exp(-OVERLAP_PARAM_Y * (VOL_FRACTION_V2S / (1.0 - VOL_FRACTION_V2S)));
        
        return fractional_escape * hydration_penalty;
    }

    /**
     * @brief Executes the controlled elution for the current time step.
     * @param dt_seconds The time delta since last tick
     * @return The amount of payload released in this tick
     */
    double tick_elution(double dt_seconds) {
        double d_ratio = calculate_fickian_efficiency();
        
        // Elution rate is proportional to diffusion efficiency and remaining gradient
        double release_rate = 0.8 * d_ratio * current_payload_pct; 
        double release_amount = release_rate * (dt_seconds / 60.0); // scaled per minute

        release_amount = std::clamp(release_amount, 0.0, current_payload_pct);
        current_payload_pct -= release_amount;

        return release_amount;
    }

    // --- Observability Getters ---
    [[nodiscard]] double get_mesh_size() const { return current_mesh_size_nm; }
    [[nodiscard]] double get_payload_remaining() const { return current_payload_pct; }
    [[nodiscard]] bool is_steric_cage_breached() const { return current_mesh_size_nm > HYDRODYNAMIC_RADIUS_NM; }
};

} // namespace metabojoint
} // namespace domains
} // namespace ai_iv
