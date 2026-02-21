#pragma once

#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

namespace ivsys {

class Utils {
public:
    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

    static double sigmoid(double x, double center = 0.0, double steepness = 1.0) {
        return 1.0 / (1.0 + std::exp(-steepness * (x - center)));
    }

    static double exponential_decay(double x, double rate = 1.0) {
        return std::exp(-rate * x);
    }

    static double gaussian(double x, double center, double sigma) {
        if (sigma <= 0.0) return 0.0;  // Prevent division by zero
        double z = (x - center) / sigma;
        return std::exp(-0.5 * z * z);
    }

    static std::string timestamp_str(std::chrono::steady_clock::time_point t) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            t.time_since_epoch()).count();
        auto secs = ms / 1000;
        auto millis = ms % 1000;

        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(6) << secs
            << "." << std::setw(3) << millis;
        return oss.str();
    }

    static long long epoch_ms() {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return static_cast<long long>(ms);
    }
};

} // namespace ivsys
