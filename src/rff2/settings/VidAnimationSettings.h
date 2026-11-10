//
// Created by Merutilm on 2025-05-08.
//

#pragma once
#include <cstdint>
#include <string>

namespace merutilm::rff2 {
    struct VidAnimationSettings {
        float overZoom;
        bool showText;
        float mps;
        std::string zoomCounterLabel = "Zoom";
        int32_t zoomCounterFont = 1; // OpenCV FONT_HERSHEY_PLAIN
    };
}
