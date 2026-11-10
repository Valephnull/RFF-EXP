#pragma once
#include <cstdint>
#include <bit>
#include <glm/vec4.hpp>

namespace merutilm::rff2 {
    struct ShdGridSettings {
        bool use = false;
        uint32_t subdivision = 10;
        float spacing = 128;
        float lineWidth = 1;
        float opacity = 0.35f;
        glm::vec4 color = {1, 1, 1, 1};

        [[nodiscard]] bool valid() const {
            const auto finite = [](const float &v) { return (std::bit_cast<uint32_t>(v) & 0x7f800000u) != 0x7f800000u; };
            return subdivision >= 2 && subdivision <= 64 && finite(spacing) && spacing >= 16 && spacing <= 2048 &&
                   finite(lineWidth) && lineWidth >= 0.5f && lineWidth <= 8 && finite(opacity) && opacity >= 0 && opacity <= 1 &&
                   finite(color.x) && finite(color.y) && finite(color.z) && finite(color.w) &&
                   color.x >= 0 && color.x <= 1 && color.y >= 0 && color.y <= 1 &&
                   color.z >= 0 && color.z <= 1 && color.w >= 0 && color.w <= 1;
        }
    };
}
