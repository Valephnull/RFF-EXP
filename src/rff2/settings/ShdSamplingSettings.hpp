//
// Iteration-buffer sampling controls shared by the live and video renderers.
//

#pragma once

#include <cstdint>

namespace merutilm::rff2 {
    struct ShdSamplingSettings {
        bool bilinear;
        uint32_t sampleCount;
    };
} // namespace merutilm::rff2
