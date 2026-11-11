#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <string>

#include "../settings/VidAnimationSettings.h"

namespace merutilm::rff2 {
    [[nodiscard]] inline std::string formatZoomCounter(const VidAnimationSettings &animation,
                                                       const double logZoom) {
        const int precision = std::clamp(animation.zoomCounterPrecision, 0, 15);
        const std::string prefix = animation.zoomCounterLabel.empty()
                                           ? std::string{}
                                           : animation.zoomCounterLabel + " : ";
        return std::format("{}{:.{}f}E{:d}", prefix, std::pow(10, std::fmod(logZoom, 1.0)), precision,
                           static_cast<int64_t>(logZoom));
    }
}
