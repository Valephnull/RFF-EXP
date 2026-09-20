#pragma once

#include <cstdint>
#include <filesystem>

#include "../settings/Settings.h"

namespace merutilm::rff2 {
    // RFF_Super-inspired full configuration and shader-preset files.  The payload is
    // versioned independently because RFF-EXP has settings that RFF_Super does not.
    struct RFFSettingsIO final {
        static constexpr uint32_t CONFIG_MAGIC = 0x52464543; // "RFEC"
        static constexpr uint32_t SHADER_MAGIC = 0x52464553; // "RFES"
        static constexpr uint32_t VERSION = 1;

        RFFSettingsIO() = delete;

        static bool saveConfig(const std::filesystem::path &path, const Settings &settings,
                               uint32_t width, uint32_t height);
        static bool loadConfig(const std::filesystem::path &path, Settings &settings,
                               uint32_t *width, uint32_t *height);

        static bool saveShaderPreset(const std::filesystem::path &path, const ShaderSettings &shader);
        static bool loadShaderPreset(const std::filesystem::path &path, ShaderSettings &shader);
    };
} // namespace merutilm::rff2
