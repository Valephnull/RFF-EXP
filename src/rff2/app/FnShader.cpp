//
// Created by Merutilm on 2025-05-16.
//

#include "FnShader.hpp"
#include "../util/ImGuiUtils.hpp"
#include "../util/Utilities.h"
#include "imgui.h"

#include "../io/KFRColorLoader.hpp"
#include "../io/GradientLoader.hpp"

namespace merutilm::rff2 {

    void FnShader::palette(RFF2 &app) {

        ImGuiUtils::BeginSettings("Palette");

        auto &[colors, interiorColor, iterationColoring, singleIterationColoring, iterationInterval, offsetRatio,
               animationSpeed] = app.getSettings().shader.palette;

        if (ImGui::DragFloat("Iteration Interval", &iterationInterval, Constants::UI::UNLIMITED_DRAG_SPEED,
                             Constants::UI::UNLIMITED_MIN_DRAG_SCALAR, Constants::UI::UNLIMITED_MAX_DRAG,
                             Constants::UI::UNLIMITED_FMT_DRAG, ImGuiSliderFlags_Logarithmic)) {
            iterationInterval = std::max(iterationInterval, FLT_MIN);
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Required iterations for the palette to cycle once");

        ImGuiUtils::BeginSettings("Colors");
        static int selected = -1;
        static int selectRequest = -1;
        static int page = 0;

        if (selected >= 0)
            selected = std::clamp(selected, 0, static_cast<int>(colors.size()) - 1);

        const bool addCheck = colors.size() < Constants::Fractal::MAX_PALETTE_LEN;
        const bool removeCheck = selected != -1 && colors.size() > 1;
        const bool clearCheck = colors.size() > 1;

        if (ImGui::BeginTable("##table", 3)) {
            ImGui::TableNextColumn();
            if (!addCheck) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("Add", ImVec2(-FLT_MIN, 0))) {
                colors.emplace_back(1, 1, 1, 1);
                page = (static_cast<int>(colors.size()) - 1) / Constants::UI::PALETTE_COLORS_PER_PAGE;
                selectRequest = static_cast<int>(colors.size()) - 1;
                app.getRequests().requestShader();
            }
            if (!addCheck) {
                ImGui::EndDisabled();
            }
            ImGui::TableNextColumn();
            if (!removeCheck) {
                ImGui::BeginDisabled();
            }


            if (ImGui::Button("Remove", ImVec2(-FLT_MIN, 0))) {
                colors.erase(colors.begin() + selected);
                app.getRequests().requestShader();

                if (selected >= colors.size())
                    selectRequest = static_cast<int>(colors.size()) - 1;
            }

            if (!removeCheck) {
                ImGui::EndDisabled();
            }
            ImGui::TableNextColumn();
            if (!clearCheck) {
                ImGui::BeginDisabled();
            }


            if (ImGui::Button("Clear", ImVec2(-FLT_MIN, 0))) {
                colors.clear();
                colors.emplace_back(1, 1, 1, 1);
                selectRequest = 0;
                app.getRequests().requestShader();
            }
            if (!clearCheck) {
                ImGui::EndDisabled();
            }

            ImGui::EndTable();
        }

        const int pageCount = (static_cast<int>(colors.size()) - 1) / Constants::UI::PALETTE_COLORS_PER_PAGE + 1;

        if (ImGui::ArrowButton("##prev", ImGuiDir_Left)) {
            if (ImGui::GetIO().KeyCtrl) {
                page -= 10;
            } else {
                --page;
            }
        }
        ImGui::SameLine();
        ImGui::Text("Page %d / %d", page + 1, pageCount);
        ImGui::SameLine();

        if (ImGui::ArrowButton("##next", ImGuiDir_Right)) {
            if (ImGui::GetIO().KeyCtrl) {
                page += 10;
            } else {
                ++page;
            }
        }
        page = std::clamp(page, 0, pageCount - 1);

        int jump = page + 1;

        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("##page", &jump, 0, 0)) {
            jump = std::clamp(jump, 1, pageCount);
            page = jump - 1;
        }

        ImGui::Separator();

        ImGuiUtils::BeginSmoothScrollableChild("##ColorList", ImVec2(-FLT_MIN, Constants::UI::PALETTE_LIST_HEIGHT));

        const int begin = page * Constants::UI::PALETTE_COLORS_PER_PAGE;
        const int end = std::min(begin + Constants::UI::PALETTE_COLORS_PER_PAGE, static_cast<int>(colors.size()));

        if (selectRequest >= 0) {
            selected = -1;
        }
        if (ImGui::BeginTable("##ColorTable", 4)) {
            for (int i = begin; i < end; ++i) {
                ImGui::TableNextColumn();
                ImGui::PushID(i);

                glm::vec4 &col = colors[i];

                std::array colRaw = {col.r, col.g, col.b};
                if (ImGui::ColorEdit3("##color", colRaw.data(),
                                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {

                    col = {colRaw[0], colRaw[1], colRaw[2], 1};
                    app.getRequests().requestShader();
                                      }

                if (ImGui::IsItemClicked())
                    selected = i;

                if (selectRequest == i) {
                    selected = selectRequest;
                    selectRequest = -1;
                    ImGui::SetScrollHereY(0.5f);
                }

                if (selected == i) {

                    ImDrawList *draw = ImGui::GetWindowDrawList();
                    draw->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), Constants::UI::SELECTED_COLOR_HIGHLIGHT,
                                  0.0f, 0, Constants::UI::SELECTED_COLOR_THICKNESS);
                }

                ImGui::SameLine();
                ImGui::Text("%d", i + 1);

                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        ImGui::EndChild();
        ImGuiUtils::EndSettings();

        std::array interiorRaw = {interiorColor.r, interiorColor.g, interiorColor.b};
        if (ImGui::ColorEdit3("Interior Color", interiorRaw.data(),
                                ImGuiColorEditFlags_NoInputs)) {
            interiorColor = {interiorRaw[0], interiorRaw[1], interiorRaw[2], 1};
            app.getRequests().requestShader();
        }

        if (ImGui::Button("Load KFR Palette", ImVec2(-FLT_MIN, 0))) {
            const auto loaded = KFRColorLoader::loadPaletteSettings();
            if (loaded.empty()) {
                vkh::logger::log_err("No colors found");
            } else {
                app.getSettings().shader.palette.colors = loaded;
                app.getRequests().requestShader();
            }
        }

        static std::vector<ImportedGradient> importedGradients;
        static int selectedGradient = 0;
        static ImGuiTextFilter gradientFilter;
        static std::string gradientError;
        const auto applyGradient = [&](const ImportedGradient &gradient) {
            colors = gradient.colors;
            selected = selectRequest = -1;
            page = 0;
            app.getRequests().requestShader();
        };
        if (ImGui::Button("Load Gradient", ImVec2(-FLT_MIN, 0))) {
            gradientError.clear();
            const auto path = IOUtilities::ioFileDialog(Constants::File::DESC_GRADIENT, IOUtilities::OPEN_FILE,
                                                       Constants::File::EXT_GRADIENT);
            if (path) {
                importedGradients = GradientLoader::load(*path, gradientError);
                selectedGradient = 0;
                gradientFilter.Clear();
                if (importedGradients.size() == 1) {
                    applyGradient(importedGradients.front());
                    importedGradients.clear();
                } else if (!importedGradients.empty()) {
                    ImGui::OpenPopup("Import Gradient");
                }
            }
        }
        if (!gradientError.empty())
            ImGui::TextWrapped("%s", gradientError.c_str());

        const ImVec2 available = ImGui::GetMainViewport()->WorkSize;
        const ImVec2 maximum(available.x * 0.95f, available.y * 0.95f);
        ImGui::SetNextWindowSize(ImVec2(std::min(560.0f, maximum.x), std::min(420.0f, maximum.y)),
                                 ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(
                ImVec2(std::min(360.0f, maximum.x), std::min(300.0f, maximum.y)), maximum);
        if (ImGui::BeginPopupModal("Import Gradient", nullptr, ImGuiWindowFlags_NoScrollbar)) {
            gradientFilter.Draw("Filter", -1);
            const float footerHeight = ImGui::GetTextLineHeightWithSpacing() + 28.0f +
                                       ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y * 2;
            const float listHeight = std::max(ImGui::GetFrameHeight(), ImGui::GetContentRegionAvail().y - footerHeight);
            if (ImGui::BeginChild("Gradient list", ImVec2(0, listHeight), ImGuiChildFlags_Borders)) {
                ImGuiListClipper clipper;
                std::vector<int> visible;
                visible.reserve(importedGradients.size());
                for (int i = 0; i < static_cast<int>(importedGradients.size()); ++i) {
                    if (gradientFilter.PassFilter(importedGradients[i].name.c_str())) visible.push_back(i);
                }
                clipper.Begin(static_cast<int>(visible.size()));
                while (clipper.Step()) {
                    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                        const int i = visible[row];
                        ImGui::PushID(i);
                        if (ImGui::Selectable(importedGradients[i].name.c_str(), selectedGradient == i))
                            selectedGradient = i;
                        ImGui::PopID();
                    }
                }
            }
            ImGui::EndChild();
            if (!importedGradients.empty()) {
                const auto &gradient = importedGradients[selectedGradient];
                ImGui::TextUnformatted(gradient.name.c_str());
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", gradient.name.c_str());
                const ImVec2 origin = ImGui::GetCursorScreenPos();
                const float width = ImGui::GetContentRegionAvail().x;
                for (size_t i = 0; i < gradient.colors.size(); ++i) {
                    const auto &left = gradient.colors[i];
                    const auto &right = gradient.colors[(i + 1) % gradient.colors.size()];
                    const ImU32 c0 = ImGui::ColorConvertFloat4ToU32(ImVec4(left.r, left.g, left.b, 1));
                    const ImU32 c1 = ImGui::ColorConvertFloat4ToU32(ImVec4(right.r, right.g, right.b, 1));
                    ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
                            ImVec2(origin.x + width * static_cast<float>(i) / gradient.colors.size(), origin.y),
                            ImVec2(origin.x + width * static_cast<float>(i + 1) / gradient.colors.size(), origin.y + 28),
                            c0, c1, c1, c0);
                }
                ImGui::Dummy(ImVec2(width, 28));
                if (ImGui::Button("Import", ImVec2(120, 0))) {
                    applyGradient(gradient);
                    importedGradients.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
            }
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                importedGradients.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }


        if (ImGui::SliderFloat("Offset Ratio", &offsetRatio, 0.0f, 1.0f)) {
            offsetRatio = std::clamp(offsetRatio, 0.0f, 1.0f);
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Start offset ratio of cycling palette.");


        if (ImGui::DragFloat("Animation Speed", &animationSpeed, Constants::UI::UNLIMITED_DRAG_SPEED,
                             Constants::UI::UNLIMITED_MIN_DRAG_ANIM, Constants::UI::UNLIMITED_MAX_DRAG,
                             Constants::UI::UNLIMITED_FMT_DRAG, ImGuiSliderFlags_Logarithmic)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Color Animation Speed, The colors' offset(iterations) per second.");


        if (ImGuiUtils::Dropdown("Iteration Coloring", &iterationColoring)) {
            app.getRequests().requestShader();
        }

        if (ImGuiUtils::Dropdown("Single Iteration Coloring", &singleIterationColoring)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::EndSettings();
    }
    void FnShader::stripe(RFF2 &app) {

        ImGuiUtils::BeginSettings("Stripe");
        auto &[stripeType, firstInterval, secondInterval, opacity, offset, animationSpeed, iterationColoring] =
                app.getSettings().shader.stripe;

        if (ImGuiUtils::Dropdown("Stripe Type", &stripeType)) {
            app.getRequests().requestShader();
        }

        if (ImGui::DragFloat("Interval 1", &firstInterval, Constants::UI::UNLIMITED_DRAG_SPEED,
                             Constants::UI::UNLIMITED_MIN_DRAG_SCALAR, Constants::UI::UNLIMITED_MAX_DRAG,
                             Constants::UI::UNLIMITED_FMT_DRAG, ImGuiSliderFlags_Logarithmic)) {
            firstInterval = std::max(firstInterval, FLT_MIN);
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the first Stripe Interval");


        if (ImGui::DragFloat("Interval 2", &secondInterval, Constants::UI::UNLIMITED_DRAG_SPEED,
                             Constants::UI::UNLIMITED_MIN_DRAG_SCALAR, Constants::UI::UNLIMITED_MAX_DRAG,
                             Constants::UI::UNLIMITED_FMT_DRAG, ImGuiSliderFlags_Logarithmic)) {
            secondInterval = std::max(secondInterval, FLT_MIN);
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the second Stripe Interval");


        if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the opacity of stripes.");


        if (ImGui::DragFloat("Offset", &offset, Constants::UI::UNLIMITED_DRAG_SPEED,
                             Constants::UI::UNLIMITED_MIN_DRAG_SCALAR, Constants::UI::UNLIMITED_MAX_DRAG,
                             Constants::UI::UNLIMITED_FMT_DRAG, ImGuiSliderFlags_Logarithmic)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Start offset iteration of stripes.");

        if (ImGui::DragFloat("Animation Speed", &animationSpeed, Constants::UI::UNLIMITED_DRAG_SPEED,
                             Constants::UI::UNLIMITED_MIN_DRAG_ANIM, Constants::UI::UNLIMITED_MAX_DRAG,
                             Constants::UI::UNLIMITED_FMT_DRAG, ImGuiSliderFlags_Logarithmic)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the stripe animation speed.");
        if (ImGuiUtils::Dropdown("Iteration Coloring", &iterationColoring)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::EndSettings();
    }
    void FnShader::surface(RFF2 &app) {

        ImGuiUtils::BeginSettings("Surface");
        auto &[depth, shadowBrightness, shadowOpacity, lightZenith, lightAzimuth, lightSharpness, lightStrength,
               distortionStrength, reflectionRatio, refractionRatio, waveFrequency, waveOffset, waveSpeed,
               waveStrength] = app.getSettings().shader.surface;


        if (ImGui::DragFloat("Depth", &depth, Constants::UI::DRAG_SPEED_SLOPE, Constants::UI::MIN_DRAG_SLOPE,
                             Constants::UI::MAX_DRAG_SLOPE, Constants::UI::FMT_SLOPE, ImGuiSliderFlags_Logarithmic)) {
            app.getRequests().requestShader();
        }

        if (ImGui::SliderFloat("Shadow Brightness", &shadowBrightness, 0, 1)) {
            app.getRequests().requestShader();
        }
        if (ImGui::SliderFloat("Shadow Opacity", &shadowOpacity, 0, 1)) {
            app.getRequests().requestShader();
        }
        ImGui::Separator();

        if (ImGui::SliderFloat("Light Zenith", &lightZenith, 0, 360)) {
            app.getRequests().requestShader();
        }
        if (ImGui::SliderFloat("Light Azimuth", &lightAzimuth, 0, 360)) {
            app.getRequests().requestShader();
        }
        if (ImGui::SliderFloat("Light Sharpness", &lightSharpness, 1, 256)) {
            app.getRequests().requestShader();
        }
        if (ImGui::SliderFloat("Light Strength", &lightStrength, 0, 2)) {
            app.getRequests().requestShader();
        }

        ImGui::Separator();

        if (ImGui::SliderFloat("Distortion Strength", &distortionStrength, 0, 1)) {
            app.getRequests().requestShader();
        }
        if (ImGui::SliderFloat("Reflection Ratio", &reflectionRatio, 0, 1)) {
            app.getRequests().requestShader();
        }
        if (ImGui::SliderFloat("Refraction Ratio", &refractionRatio, 1, 3)) {
            app.getRequests().requestShader();
        }
        ImGui::Separator();

        if (ImGui::SliderFloat("Wave Frequency", &waveFrequency, 0.1f, 2)) {
            app.getRequests().requestShader();
        }
        std::array wo = {waveOffset.x, waveOffset.y};
        if (ImGui::SliderFloat2("Wave Offset", wo.data(), 0, 1)) {
            waveOffset = {wo[0], wo[1]};
            app.getRequests().requestShader();
        }
        if (ImGui::SliderFloat("Wave Speed", &waveSpeed, 0, 4)) {
            app.getRequests().requestShader();
        }
        if (ImGui::SliderFloat("Wave Strength", &waveStrength, 0, 4)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::EndSettings();
    }
    void FnShader::color(RFF2 &app) {

        ImGuiUtils::BeginSettings("Color");
        auto &[gamma, exposure, hue, saturation, brightness, contrast] = app.getSettings().shader.color;

        if (ImGui::SliderFloat("Gamma", &gamma, 0.5f, 1.5f)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the gamma.");
        if (ImGui::SliderFloat("Exposure", &exposure, -1, 1)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the exposure.");
        if (ImGui::SliderFloat("Hue", &hue, 0, 1)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the hue.");
        if (ImGui::SliderFloat("Saturation", &saturation, -1, 1)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the saturation.");
        if (ImGui::SliderFloat("Brightness", &brightness, -1, 1)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the brightness.");
        if (ImGui::SliderFloat("Contrast", &contrast, -1, 1)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the contrast.");
        ImGuiUtils::EndSettings();
    }
    void FnShader::fog(RFF2 &app) {

        ImGuiUtils::BeginSettings("Fog");
        auto &[radius, opacity] = app.getSettings().shader.fog;

        if (ImGui::SliderFloat("Radius", &radius, 0, 1)) {
            radius = std::clamp(radius, 0.0f, 1.0f);
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the radius of the fog.");
        if (ImGui::SliderFloat("Opacity", &opacity, 0, 1)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the opacity of the fog.");
        ImGuiUtils::EndSettings();
    }

    void FnShader::bloom(RFF2 &app) {

        ImGuiUtils::BeginSettings("Bloom");
        auto &[threshold, radius, softness, intensity] = app.getSettings().shader.bloom;

        if (ImGui::SliderFloat("Threshold", &threshold, 0, 1)) {
            threshold = std::clamp(threshold, 0.0f, 1.0f);
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the threshold of the bloom.");
        if (ImGui::SliderFloat("Radius", &radius, 0, 1)) {
            radius = std::clamp(radius, 0.0f, 1.0f);
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the radius of the bloom.");
        if (ImGui::SliderFloat("Softness", &softness, 0, 1)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the softness of the bloom.");
        if (ImGui::SliderFloat("Intensity", &intensity, 0, 1)) {
            app.getRequests().requestShader();
        }
        ImGuiUtils::HelpMarker("Sets the intensity of the bloom.");
        ImGuiUtils::EndSettings();
    }
    void FnShader::grid(RFF2 &app) {
        ImGuiUtils::BeginSettings("Grid");
        auto &grid = app.getSettings().shader.grid;
        ImGui::Checkbox("Enabled", &grid.use);
        if (grid.use) {
            int subdivision = static_cast<int>(grid.subdivision);
            if (ImGui::InputInt("Subdivision", &subdivision))
                grid.subdivision = static_cast<uint32_t>(std::clamp(subdivision, 2, 64));
            ImGuiUtils::HelpMarker("Divisions per cell at each level, for example 2, 4, or 10.");
            ImGui::SliderFloat("Cell Spacing", &grid.spacing, 16, 512, "%.0f px");
            ImGui::SliderFloat("Line Width", &grid.lineWidth, 0.5f, 8, "%.1f px");
            ImGui::SliderFloat("Opacity", &grid.opacity, 0, 1);
            ImGui::ColorEdit3("Line Color", &grid.color.x, ImGuiColorEditFlags_NoInputs);
        }
        ImGuiUtils::EndSettings();
    }

    void FnShader::sampling(RFF2 &app) {
        ImGuiUtils::BeginSettings("Sampling");
        auto &[bilinear, sampleCount] = app.getSettings().shader.sampling;
        if (ImGui::Checkbox("Bilinear", &bilinear))
            app.getRequests().requestShader();

        constexpr uint32_t minimumSamples = 1;
        constexpr uint32_t maximumSamples = 256;
        if (ImGui::InputScalar("Samples", ImGuiDataType_U32, &sampleCount)) {
            sampleCount = std::clamp(sampleCount, minimumSamples, maximumSamples);
            app.getRequests().requestShader();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Stochastic samples per pixel (1-256)");
        ImGuiUtils::EndSettings();
    }
#ifdef USE_EXPERIMENTAL_3D
    void FnShader::fractal3D(RFF2 &app) {


        ImGuiUtils::BeginSettings("3D  (Wow epic)");
        auto &[use, altitude, rotation, distance, baseIteration, divisor] = app.getSettings().shader.fractal3D;

        ImGui::Checkbox("Use", &use);

        if (ImGui::SliderFloat("Altitude", &altitude, 5, 89.99f)) {
            app.getRequests().requestShader();
        }

        if (ImGui::SliderFloat("Rotation", &rotation, 0, 360)) {
            app.getRequests().requestShader();
        }

        if (ImGui::SliderFloat("Distance", &distance, 0.2f, 2.0f)) {
            app.getRequests().requestShader();
        }


        if (ImGui::DragFloat("Base Iteration", &baseIteration, Constants::UI::UNLIMITED_DRAG_SPEED,
                             Constants::UI::UNLIMITED_MIN_DRAG_SCALAR, Constants::UI::UNLIMITED_MAX_DRAG,
                             Constants::UI::UNLIMITED_FMT_DRAG, ImGuiSliderFlags_Logarithmic)) {
            app.getRequests().requestShader();
        }


        if (ImGui::DragFloat("Depth Divisor", &divisor, Constants::UI::UNLIMITED_DRAG_SPEED,
                             Constants::UI::UNLIMITED_MIN_DRAG_SCALAR, Constants::UI::UNLIMITED_MAX_DRAG,
                             Constants::UI::UNLIMITED_FMT_DRAG, ImGuiSliderFlags_Logarithmic)) {
            divisor = std::max(divisor, FLT_MIN);
            app.getRequests().requestShader();
        }
        ImGuiUtils::EndSettings();
    }
#endif


} // namespace merutilm::rff2
