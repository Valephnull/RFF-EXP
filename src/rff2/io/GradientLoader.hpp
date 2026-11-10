#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <glm/vec4.hpp>

namespace merutilm::rff2 {
    struct ImportedGradient {
        std::string name;
        std::vector<glm::vec4> colors;
    };

    // Text gradient packs containing named { gradient: index=... color=... } entries.
    // Imported colors are sampled over the format's cyclic 400-position palette.
    struct GradientLoader final {
        static std::vector<ImportedGradient> load(const std::filesystem::path &path, std::string &error);
        static std::vector<ImportedGradient> parse(std::string_view text, std::string &error);
    };
}
