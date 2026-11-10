#include <cassert>
#include <filesystem>
#include <fstream>

#include "rff2/io/RFFSettingsIO.h"
#include "rff2/mb/Perturbator.h"

using namespace merutilm::rff2;

namespace {
    Settings makeSettings() {
        return Settings{
                .fractal = {
                        .general = {.bailout = 4.0f, .logZoom = 12.5, .threads = 1},
                        .reference = {
                                .center = fixed_point_complex("-0.743643887037151", "0.13182590420533",
                                                                 Perturbator::logZoomToExp10(12.5f)),
                                .useParallelRefCalculation = true,
                                .sync = {.referenceSynchronizationInterval = 7,
                                         .referenceSynchronizationRadiusPower = 5},
                                .compression = {.minCompressSize = 1234, .compressionThresholdPower = 9},
                                .reuse = true,
                                .useFixedPrecision = true,
                                .partitionSize = 1024,
                                .fixedPrecisionNeg = 32,
                        },
                        .sa = {.use = true, .appliedTermsCount = 9, .validatedTermsCount = 2, .epsilonPower = -6.0f},
                        .mpa = {.minSkipReference = 8,
                                .maxMultiplierBetweenLevel = 4,
                                .epsilonPower = -7.0f,
                                .selectionMethod = FrtMPASelectionMethod::HIGHEST},
                        .perturb = {.maxIteration = (itt::ptb{1} << 90) + 123456,
                                    .decimalizeIterationMethod = FrtDecimalizeIterationMethod::LOG,
                                    .autoMaxIteration = false,
                                    .interiorDetectRadiusPower = 8,
                                    .autoIterationMultiplier = 75,
                                    .absoluteIterationMode = true}},
                .render = {
                        .display = {.clarityMultiplier = 1.75f,
                                    .fps = 144.0f,
                                    .blockSize = 32},
                        .computeShader = {.use = true,
                                          .preferredBatchDuration = 0.25f,
                                          .allowedGlitchPixelCount = 42,
                                          .completelyIgnoreMpa = true,
                                          .automaticAcceptMpaBatches = 6,
                                          .interpolateIsolated = true}},
                .shader = {
                        .palette = {.colors = {{0.1f, 0.2f, 0.3f, 1.0f}, {0.8f, 0.7f, 0.6f, 1.0f}},
                                    .interiorColor = {0.01f, 0.02f, 0.03f, 1.0f},
                                    .iterationColoring = ShdIterationColoringMethod::LOG,
                                    .singleIterationColoring = ShdPalSingleIterationColoringMethod::REVERSED,
                                    .iterationInterval = 17.0f,
                                    .offsetRatio = 0.25f,
                                    .animationSpeed = 2.0f},
                        .stripe = {.stripeType = ShdStripeType::SMOOTH,
                                   .firstInterval = 5.0f,
                                   .secondInterval = 7.0f,
                                   .opacity = 0.4f,
                                   .offset = 0.2f,
                                   .animationSpeed = 0.5f,
                                   .iterationColoring = ShdIterationColoringMethod::SQUARE_ROOT},
                        .surface = {.depth = 1.0f,
                                    .shadowBrightness = 0.3f,
                                    .shadowOpacity = 0.6f,
                                    .lightZenith = 45.0f,
                                    .lightAzimuth = 120.0f,
                                    .lightSharpness = 64.0f,
                                    .lightStrength = 0.8f,
                                    .distortionStrength = 0.2f,
                                    .reflectionRatio = 0.04f,
                                    .refractionRatio = 1.5f,
                                    .waveFrequency = 1.0f,
                                    .waveOffset = {0.1f, 0.2f},
                                    .waveSpeed = 1.0f,
                                    .waveStrength = 0.5f},
                        .color = {.gamma = 1.1f,
                                  .exposure = 0.2f,
                                  .hue = 0.3f,
                                  .saturation = 1.2f,
                                  .brightness = 0.1f,
                                  .contrast = 0.4f},
                        .fog = {.radius = 3.0f, .opacity = 0.2f},
                        .bloom = {.threshold = 0.7f, .radius = 6.0f, .softness = 0.5f, .intensity = 0.8f},
                        .sampling = {.bilinear = true, .sampleCount = 32},
                        .fractal3D = {.use = true,
                                      .altitude = 70.0f,
                                      .rotation = 12.0f,
                                      .distance = 1.5f,
                                      .baseIteration = 4.0f,
                                      .depthDivisor = 9.0f},
                        .grid = {.use = true, .subdivision = 4, .spacing = 96, .lineWidth = 2,
                                 .opacity = 0.6f, .color = {0.2f, 0.4f, 0.8f, 1}}},
                .video = {.data = {.defaultZoomIncrement = 2.5f, .isStatic = true},
                          .animation = {.overZoom = 1.5f, .showText = false, .mps = 2.0f,
                                        .zoomCounterLabel = "Magnification", .zoomCounterFont = 4},
                          .exportation = {.fps = 30.0f, .bitrate = 12000}},
                .explore = {.autoMoveCursorToCenter = false,
                            .autoAimRadiusPixels = 137,
                            .locator = {.burst = true}}};
    }
}

int main() {
    const std::filesystem::path base = std::filesystem::temp_directory_path() / "rff_exp_settings_io_test";
    const std::filesystem::path configPath = base.string() + ".rfc";
    const std::filesystem::path shaderPath = base.string() + ".rfsp";

    const Settings original = makeSettings();
    assert(RFFSettingsIO::saveConfig(configPath, original, 1920, 1080));

    Settings loaded = makeSettings();
    loaded.fractal.general.logZoom = 2.0f;
    loaded.shader.palette.colors = {{1.0f, 1.0f, 1.0f, 1.0f}};
    uint32_t width = 0;
    uint32_t height = 0;
    assert(RFFSettingsIO::loadConfig(configPath, loaded, &width, &height));
    assert(width == 1920 && height == 1080);
    assert(loaded.fractal.general.logZoom == original.fractal.general.logZoom);
    assert(loaded.fractal.reference.center.clone_real().to_string() ==
           original.fractal.reference.center.clone_real().to_string());
    assert(loaded.fractal.reference.center.clone_imag().to_string() ==
           original.fractal.reference.center.clone_imag().to_string());
    assert(loaded.fractal.perturb.maxIteration == original.fractal.perturb.maxIteration);
    assert(loaded.fractal.reference.partitionSize == original.fractal.reference.partitionSize);
    assert(loaded.render.computeShader.allowedGlitchPixelCount == 42);
    assert(loaded.shader.palette.colors.size() == 2);
    assert(loaded.shader.palette.colors[1].b == original.shader.palette.colors[1].b);
    assert(loaded.shader.sampling.sampleCount == 32);
    assert(loaded.shader.grid.use && loaded.shader.grid.subdivision == 4);
    assert(loaded.shader.grid.color == original.shader.grid.color);
    assert(loaded.shader.fractal3D.use);
    assert(loaded.video.exportation.bitrate == 12000);
    assert(loaded.video.animation.zoomCounterLabel == "Magnification");
    assert(loaded.video.animation.zoomCounterFont == 4);
    assert(loaded.explore.autoAimRadiusPixels == 137);

    // Version 5 configs ended immediately before the counter label/font tail.
    // They should retain the historical defaults when loaded by version 6.
    std::filesystem::resize_file(configPath, std::filesystem::file_size(configPath) -
                                             sizeof(uint64_t) - original.video.animation.zoomCounterLabel.size() -
                                             sizeof(int32_t));
    {
        std::fstream legacy(configPath, std::ios::binary | std::ios::in | std::ios::out);
        legacy.seekp(sizeof(uint32_t));
        const uint32_t version = 5;
        legacy.write(reinterpret_cast<const char *>(&version), sizeof(version));
    }
    loaded.video.animation.zoomCounterLabel = "Should be reset";
    loaded.video.animation.zoomCounterFont = 7;
    assert(RFFSettingsIO::loadConfig(configPath, loaded, &width, &height));
    assert(loaded.video.animation.zoomCounterLabel == "Zoom");
    assert(loaded.video.animation.zoomCounterFont == 1);

    assert(RFFSettingsIO::saveShaderPreset(shaderPath, original.shader));
    ShaderSettings shader = loaded.shader;
    shader.sampling.sampleCount = 1;
    shader.palette.colors = {{0.0f, 0.0f, 0.0f, 1.0f}};
    assert(RFFSettingsIO::loadShaderPreset(shaderPath, shader));
    assert(shader.palette.colors.size() == 2);
    assert(shader.sampling.sampleCount == 32);
    assert(shader.stripe.stripeType == ShdStripeType::SMOOTH);
    assert(shader.fractal3D.depthDivisor == 9.0f);
    assert(shader.grid.use && shader.grid.spacing == 96 && shader.grid.lineWidth == 2);
    assert(shader.grid.opacity == original.shader.grid.opacity);

    // v4 presets have the same prefix, with no grid tail. Loading one must
    // disable any grid from the previously loaded preset.
    std::filesystem::resize_file(shaderPath, std::filesystem::file_size(shaderPath) - 33);
    {
        std::fstream legacy(shaderPath, std::ios::binary | std::ios::in | std::ios::out);
        legacy.seekp(sizeof(uint32_t));
        const uint32_t version = 4;
        legacy.write(reinterpret_cast<const char *>(&version), sizeof(version));
    }
    assert(RFFSettingsIO::loadShaderPreset(shaderPath, shader));
    assert(!shader.grid.use && shader.grid.subdivision == 10);

    std::filesystem::remove(configPath);
    std::filesystem::remove(shaderPath);
    return 0;
}
