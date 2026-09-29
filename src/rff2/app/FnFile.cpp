//
// Created by Merutilm on 2025-05-14.
//

#include "FnFile.hpp"

#include "../app/RFF2.hpp"
#include "../constants/Constants.hpp"
#include "../io/RFFBinary.hpp"
#include "../io/RFFSettingsIO.h"
#include "../util/Utilities.h"
#include "IOUtilities.h"
#include "imgui.h"
#include "vulkan_helper/base/logger.hpp"

namespace merutilm::rff2 {

    void FnFile::saveShader(RFF2 &app) {
        if (ImGui::Button("Save Shader", ImVec2(-FLT_MIN, 0))) {
            const auto path = IOUtilities::ioFileDialog(Constants::File::DESC_SHADER, IOUtilities::SAVE_FILE,
                                                        Constants::File::EXT_SHADER);
            if (path)
                app.saveCurrentShader(*path);
        }
    }

    void FnFile::saveMap(RFF2 &app) {
        if (ImGui::Button("Save Map", ImVec2(-FLT_MIN, 0))) {
            const auto path = IOUtilities::ioFileDialog(Constants::File::DESC_DYNAMIC_MAP, IOUtilities::SAVE_FILE,
                                                        Constants::File::EXT_DYNAMIC_MAP);
            if (path == nullptr) {
                return;
            }
            RFFBinary::exportFile(app.generateMap(), *path);
        }
    }
    void FnFile::saveImage(RFF2 &app) {
        if (ImGui::Button("Save Image", ImVec2(-FLT_MIN, 0))) {
            app.getRequests().requestCreateImage();
        }
    }
    void FnFile::saveLocation(RFF2 &app) {

        if (ImGui::Button("Save Location", ImVec2(-FLT_MIN, 0))) {
            const auto path = IOUtilities::ioFileDialog(Constants::File::DESC_LOCATION, IOUtilities::SAVE_FILE,
                   Constants::File::EXT_LOCATION);
            if (path == nullptr) {
                return;
            }
            app.saveCurrentLocation(*path);
        }
    }

    void FnFile::saveSettings(RFF2 &app) {
        if (!ImGui::Button("Save Settings", ImVec2(-FLT_MIN, 0)))
            return;
        const auto path = IOUtilities::ioFileDialog(Constants::File::DESC_CONFIG, IOUtilities::SAVE_FILE,
                                                     Constants::File::EXT_CONFIG);
        if (!path)
            return;
        const VkExtent2D extent = app.getWindowContext().getSwapchain().getSwapchainExtent();
        if (!RFFSettingsIO::saveConfig(*path, app.getSettings(), extent.width, extent.height))
            vkh::logger::log_err("Failed to save settings");
    }

    void FnFile::saveShaderPreset(RFF2 &app) {
        if (!ImGui::Button("Save Shader Preset", ImVec2(-FLT_MIN, 0)))
            return;
        const auto path = IOUtilities::ioFileDialog(Constants::File::DESC_SHADER_PRESET, IOUtilities::SAVE_FILE,
                                                     Constants::File::EXT_SHADER_PRESET);
        if (path && !RFFSettingsIO::saveShaderPreset(*path, app.getSettings().shader))
            vkh::logger::log_err("Failed to save shader preset");
    }
    void FnFile::loadMap(RFF2 &app) {

        if (ImGui::Button("Load Map", ImVec2(-FLT_MIN, 0))) {
            const auto path = IOUtilities::ioFileDialog(Constants::File::DESC_DYNAMIC_MAP, IOUtilities::OPEN_FILE,
                        Constants::File::EXT_DYNAMIC_MAP);
            if (path == nullptr) {
                return;
            }
            app.overwriteMatrixFromMap(RFFBinary::importFile<RFFDynamicMapBinary>(*path));
        }
    }

    void FnFile::loadLocation(RFF2 &app) {


        if (ImGui::Button("Load Location", ImVec2(-FLT_MIN, 0))) {
            const auto path = IOUtilities::ioFileDialog(Constants::File::DESC_LOCATION, IOUtilities::OPEN_FILE,
                   Constants::File::EXT_LOCATION);
            if (path == nullptr) {
                return;
            }
            app.loadLocation(*path);
        }
    }

    void FnFile::loadSettings(RFF2 &app) {
        if (!ImGui::Button("Load Settings", ImVec2(-FLT_MIN, 0)))
            return;
        const auto path = IOUtilities::ioFileDialog(Constants::File::DESC_CONFIG, IOUtilities::OPEN_FILE,
                                                     Constants::File::EXT_CONFIG);
        if (!path)
            return;

        Settings loaded = app.getSettings();
        uint32_t width = 0;
        uint32_t height = 0;
        if (!RFFSettingsIO::loadConfig(*path, loaded, &width, &height)) {
            vkh::logger::log_err("Failed to load settings");
            return;
        }

        // A reused reference belongs to the old in-memory location and cannot travel
        // with a settings file. Always calculate a matching reference after loading.
        loaded.fractal.reference.reuse = false;
        app.getSettings() = std::move(loaded);
        app.getWindowContext().getWindow()->initializerSettings.framerate = app.getSettings().render.display.fps;
        app.getWindowContext().getWindow()->setResolution(static_cast<int>(width), static_cast<int>(height));
        app.getRequests().requestShader();
        app.getRequests().requestResize({width, height});
        app.getRequests().requestRecompute();
    }

    void FnFile::loadShaderPreset(RFF2 &app) {
        if (!ImGui::Button("Load Shader Preset", ImVec2(-FLT_MIN, 0)))
            return;
        const auto path = IOUtilities::ioFileDialog(Constants::File::DESC_SHADER_PRESET, IOUtilities::OPEN_FILE,
                                                     Constants::File::EXT_SHADER_PRESET);
        if (!path)
            return;
        if (!RFFSettingsIO::loadShaderPreset(*path, app.getSettings().shader)) {
            vkh::logger::log_err("Failed to load shader preset");
            return;
        }
        app.getRequests().requestShader();
    }

    void FnFile::loadShader(RFF2 &app) {
        if (ImGui::Button("Load Shader", ImVec2(-FLT_MIN, 0))) {
            const auto path = IOUtilities::ioFileDialog(Constants::File::DESC_SHADER, IOUtilities::OPEN_FILE,
                                                        Constants::File::EXT_SHADER);
            if (path)
                app.loadShader(*path);
        }
    }

    void FnFile::autoSaveBackup(RFF2 &app) {
        ImGui::Checkbox("Auto Save Backup", &app.getSettings().file.autoSaveBackup);
        Utilities::imguiHelpMarker("Automatically saves the backup for each calculation");
    }
} // namespace merutilm::rff2
