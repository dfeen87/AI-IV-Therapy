#pragma once

/*
 * config_defaults.hpp
 *
 * Canonical default configuration values for the
 * AI-Optimized Intravenous Therapy Control System.
 *
 * This file exists to:
 * - Make experimental assumptions explicit
 * - Support reproducibility
 * - Centralize tunable parameters
 *
 * No control logic is defined here.
 */

namespace ivsys {
namespace config {

// -----------------------------
// Control Loop Timing
// -----------------------------
constexpr int CONTROL_FREQUENCY_HZ = 5;
constexpr double CONTROL_PERIOD_SEC = 0.2;

// -----------------------------
// Infusion Rate Bounds
// -----------------------------
constexpr double MIN_INFUSION_RATE_ML_MIN = 0.1;
constexpr double MAX_INFUSION_RATE_ML_MIN = 1.5;

// -----------------------------
// Risk Thresholds
// -----------------------------
constexpr double LOW_RISK_MAX = 0.3;
constexpr double MODERATE_RISK_MAX = 0.6;
constexpr double HIGH_RISK_THRESHOLD = 0.75;

// -----------------------------
// Cardiac Safety
// -----------------------------
constexpr double MIN_CARDIAC_RESERVE = 0.2;
constexpr double CARDIAC_LIMIT_THRESHOLD = 0.3;

// -----------------------------
// Rate-of-Change Safety
// -----------------------------
constexpr double MAX_RATE_CHANGE_ML_MIN = 0.3;  // ml/min per control cycle

// -----------------------------
// Energy Transfer Model
// -----------------------------
constexpr double BASELINE_METABOLIC_POWER_W = 100.0;
constexpr double IV_SUPPLEMENT_POWER_W = 35.0;

constexpr double DEFAULT_OPTIMAL_FLOW_CM_S = 20.0;
constexpr double DEFAULT_FLOW_SIGMA_CM_S = 5.0;

// -----------------------------
// Prediction
// -----------------------------
constexpr int PREDICTION_HORIZON_MIN = 10;
constexpr double UNCERTAINTY_GROWTH_PER_MIN = 0.05;

// -----------------------------
// Volume Safety
// -----------------------------
constexpr double DAILY_VOLUME_PER_KG_ML = 35.0;

// -----------------------------
// Tachycardia Detection
// -----------------------------
constexpr double TACHYCARDIA_HR_MULTIPLIER = 1.4;

// -----------------------------
// Safety Rate Caps
// -----------------------------
constexpr double VOLUME_LIMIT_RATE_CAP     = 0.3;
constexpr double LOW_CARDIAC_RATE_CAP      = 0.5;
constexpr double HIGH_RISK_RATE_CAP        = 0.6;
constexpr double TACHYCARDIA_RATE_CAP      = 0.4;
constexpr double EMERGENCY_MIN_RATE        = 0.1;
constexpr double EMERGENCY_HYDRATION_THRESHOLD = 50.0;

// -----------------------------
// Volume Overload Threshold
// -----------------------------
constexpr double VOLUME_APPROACH_FRACTION  = 0.9;

// -----------------------------
// Predictive Boost
// -----------------------------
constexpr double PREDICTIVE_BOOST_MULTIPLIER     = 1.2;
constexpr double PREDICTIVE_HYDRATION_THRESHOLD  = 50.0;

// -----------------------------
// Cardiac Limiting
// -----------------------------
constexpr double CARDIAC_SCALING_BASE      = 0.5;
constexpr double CARDIAC_SCALING_RANGE     = 0.5;
constexpr double CARDIAC_SIGMOID_STEEPNESS = 10.0;

// -----------------------------
// Daily Volume Condition Factors
// -----------------------------
constexpr double CARDIAC_CONDITION_VOLUME_FACTOR = 0.7;
constexpr double RENAL_IMPAIRMENT_VOLUME_FACTOR  = 0.6;

} // namespace config
} // namespace ivsys
