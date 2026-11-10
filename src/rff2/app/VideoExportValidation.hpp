#pragma once

#include <bit>
#include <cstdint>
#include <string>

namespace merutilm::rff2 {
    // Release uses fast-math, so do not use isfinite (which it can optimize away).
    inline bool finiteVideoValue(const double &value) {
        return (std::bit_cast<uint64_t>(value) & UINT64_C(0x7ff0000000000000)) !=
               UINT64_C(0x7ff0000000000000);
    }

    inline std::string validateVideoExport(const double &fps, const double &speed, const double &overZoom,
                                          const double &zoomIncrement, const uint32_t width, const uint32_t height) {
        if (!finiteVideoValue(fps) || fps <= 0)
            return "FPS must be a finite number greater than zero.";
        if (!finiteVideoValue(speed) || speed <= 0)
            return "Zoom Speed must be a finite number greater than zero.";
        if (!finiteVideoValue(overZoom) || overZoom < 0)
            return "Over Zoom must be a finite number greater than or equal to zero.";
        if (!finiteVideoValue(zoomIncrement) || zoomIncrement <= 1)
            return "Default Zoom Increment must be a finite number greater than one.";
        if (width == 0 || height == 0 || width % 2 || height % 2)
            return "H.264 export requires a nonzero, even keyframe width and height (including Clarity).";
        return {};
    }
}
