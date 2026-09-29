#include <cassert>
#include <filesystem>

#include "rff2/io/RFFSettingsIO.h"
#include "rff2/mb/Perturbator.h"

using namespace merutilm::rff2;

namespace {
    Settings makeSettings() {
        return Settings{
                .fractal = {
                        .general = {.bailout = 4.0f, .logZoom = 12.5f, .threads = 1},
                        .reference = {
                                .center = fixed_point_complex("-0.743643887037151", "0.13182590420533",
                                                                 Perturbator::logZoomToExp10(12.5f)),
                                .useParallelRefCalculation = true,
                                .sync = {.referenceSynchronizationInterval = 7,
                                         .referenceSynchronizationRadiusPower = 5},
                                .compression = {.compressCriteria = 1234, .compressionThresholdPower = 9},
                                .reuse = true,
                                .useFixedPrecision = true,
                                .fixedPrecisionNeg = 32,
                        },
                        .sa = {.use = true, .appliedTermsCount = 9, .validatedTermsCount = 2, .epsilonPower = -6.0f},
                        .mpa = {.minSkipReference = 8,
                                .maxMultiplierBetweenLevel = 4,
                                .epsilonPower = -7.0f,
                                .selectionMethod = FrtMPASelectionMethod::HIGHEST,
                                .useCompress = true,
                                .useParallelization = true},
                        .perturb = {.maxIteration = 123456,
                                    .decimalizeIterationMethod = FrtDecimalizeIterationMethod::LOG,
                                    .autoMaxIteration = false,
                                    .interiorDetectRadiusPower = 8,
                                    .autoIterationMultiplier = 75,
                                    .absoluteIterationMode = true}},
                .render = {
                        .display = {.clarityMultiplier = 1.75f,
                                    .fps = 144.0f,
                                    .pixelRenderPriority = RndPixelRenderPriority::SWIZZLE},
                        .computeShader = {.use = true,
                                          .preferredBatchDuration = 0.25f,
                                          .allowedGlitchPixelCount = 42,
                                          .completelyIgnoreMpa = true,
                                          .automaticAcceptMpaBatches = 6,
                                          .interpolateIsolated = true}},
                .shader = {
                        .palette = {.colors = {{0.1f, 0.2f, 0.3f, 1.0f}, {0.8f, 0.7f, 0.6f, 1.0f}},
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
                        .slope = {.depth = 1.0f,
                                  .reflectionRatio = 0.3f,
                                  .opacity = 0.6f,
                                  .zenith = 45.0f,
                                  .azimuth = 120.0f},
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
                                      .depthDivisor = 9.0f}},
                .video = {.data = {.defaultZoomIncrement = 2.5f, .isStatic = true},
                          .animation = {.overZoom = 1.5f, .showText = false, .mps = 2.0f},
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
    assert(loaded.render.computeShader.allowedGlitchPixelCount == 42);
    assert(loaded.shader.palette.colors.size() == 2);
    assert(loaded.shader.palette.colors[1].b == original.shader.palette.colors[1].b);
    assert(loaded.shader.sampling.sampleCount == 32);
    assert(loaded.shader.fractal3D.use);
    assert(loaded.video.exportation.bitrate == 12000);
    assert(loaded.explore.autoAimRadiusPixels == 137);

    assert(RFFSettingsIO::saveShaderPreset(shaderPath, original.shader));
    ShaderSettings shader = loaded.shader;
    shader.sampling.sampleCount = 1;
    shader.palette.colors = {{0.0f, 0.0f, 0.0f, 1.0f}};
    assert(RFFSettingsIO::loadShaderPreset(shaderPath, shader));
    assert(shader.palette.colors.size() == 2);
    assert(shader.sampling.sampleCount == 32);
    assert(shader.stripe.stripeType == ShdStripeType::SMOOTH);
    assert(shader.fractal3D.depthDivisor == 9.0f);

    std::filesystem::remove(configPath);
    std::filesystem::remove(shaderPath);
    return 0;
}
