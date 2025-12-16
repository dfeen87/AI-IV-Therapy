#pragma once

/*
 * replay_logger.hpp
 *
 * Conceptual interface for replaying and analyzing
 * logged IV control sessions.
 *
 * This file documents how telemetry and control logs
 * can be reconstructed for offline analysis, validation,
 * and visualization.
 *
 * No runtime behavior is implemented here.
 */

#include <string>

namespace ivsys {

struct ReplaySessionInfo {
    std::string session_id;
    std::string telemetry_file;
    std::string control_file;
    std::string system_log_file;
};

class ReplayLogger {
public:
    /*
     * Load a completed session for offline analysis.
     *
     * Intended use:
     * - Reconstruct control decisions
     * - Validate safety behavior
     * - Compare strategies
     * - Support scientific review
     */
    static ReplaySessionInfo load_session(const std::string& session_id);

    /*
     * Replay telemetry and control events in time order.
     *
     * Future extensions may support:
     * - accelerated playback
     * - stepwise inspection
     * - alternative controller comparison
     */
    static void replay(const ReplaySessionInfo& session);

    /*
     * Export reconstructed state for visualization or ML analysis.
     */
    static void export_reconstructed_state(const ReplaySessionInfo& session,
                                           const std::string& output_path);
};

} // namespace ivsys
