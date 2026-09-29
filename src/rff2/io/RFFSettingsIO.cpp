#include "RFFSettingsIO.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <string>
#include <thread>
#include <type_traits>

#include "../mb/Perturbator.h"

namespace merutilm::rff2 {
    namespace {
        constexpr uint64_t MAX_COORDINATE_BYTES = 16ULL * 1024 * 1024;
        constexpr uint64_t MAX_PALETTE_COLORS = 1'000'000;

        class Writer final {
            std::ofstream stream;

        public:
            explicit Writer(const std::filesystem::path &path)
                : stream(path, std::ios::out | std::ios::binary | std::ios::trunc) {}

            template<typename T>
                requires std::is_arithmetic_v<T>
            void value(const T input) {
                stream.write(reinterpret_cast<const char *>(&input), sizeof(input));
            }

            template<typename E>
                requires std::is_enum_v<E>
            void enumeration(const E input) {
                value(static_cast<int32_t>(input));
            }

            void boolean(const bool input) { value(static_cast<uint8_t>(input ? 1 : 0)); }

            void text(const std::string &input) {
                value(static_cast<uint64_t>(input.size()));
                stream.write(input.data(), static_cast<std::streamsize>(input.size()));
            }

            [[nodiscard]] bool good() const { return static_cast<bool>(stream); }

            bool finish() {
                stream.flush();
                stream.close();
                return !stream.fail();
            }
        };

        class Reader final {
            std::ifstream stream;

        public:
            explicit Reader(const std::filesystem::path &path) : stream(path, std::ios::in | std::ios::binary) {}

            template<typename T>
                requires std::is_arithmetic_v<T>
            bool value(T &output) {
                stream.read(reinterpret_cast<char *>(&output), sizeof(output));
                return static_cast<bool>(stream);
            }

            template<typename E>
                requires std::is_enum_v<E>
            bool enumeration(E &output, const int32_t minimum, const int32_t maximum) {
                int32_t raw = 0;
                if (!value(raw) || raw < minimum || raw > maximum)
                    return false;
                output = static_cast<E>(raw);
                return true;
            }

            bool boolean(bool &output) {
                uint8_t raw = 0;
                if (!value(raw) || raw > 1)
                    return false;
                output = raw != 0;
                return true;
            }

            bool text(std::string &output, const uint64_t maximumBytes = MAX_COORDINATE_BYTES) {
                uint64_t length = 0;
                if (!value(length) || length == 0 || length > maximumBytes)
                    return false;
                output.resize(static_cast<size_t>(length));
                stream.read(output.data(), static_cast<std::streamsize>(length));
                return static_cast<bool>(stream);
            }

            [[nodiscard]] bool good() const { return static_cast<bool>(stream); }
        };

        [[nodiscard]] bool finite(const float value) {
            constexpr uint32_t EXPONENT = uint32_t{0xff} << 23;
            return (std::bit_cast<uint32_t>(value) & EXPONENT) != EXPONENT;
        }

        [[nodiscard]] bool finite(const double value) {
            constexpr uint64_t EXPONENT = uint64_t{0x7ff} << 52;
            return (std::bit_cast<uint64_t>(value) & EXPONENT) != EXPONENT;
        }

        [[nodiscard]] bool finiteVec4(const glm::vec4 &value) {
            return finite(value.x) && finite(value.y) && finite(value.z) && finite(value.w);
        }

        [[nodiscard]] bool validDecimalText(const std::string &value) {
            if (value.empty())
                return false;
            size_t index = value.front() == '-' || value.front() == '+' ? 1 : 0;
            bool sawDigit = false;
            bool sawDecimalPoint = false;
            for (; index < value.size(); ++index) {
                const char character = value[index];
                if (character >= '0' && character <= '9') {
                    sawDigit = true;
                } else if (character == '.' && !sawDecimalPoint) {
                    sawDecimalPoint = true;
                } else {
                    return false;
                }
            }
            return sawDigit;
        }

        void writeVec4(Writer &writer, const glm::vec4 &value) {
            writer.value(value.x);
            writer.value(value.y);
            writer.value(value.z);
            writer.value(value.w);
        }

        bool readVec4(Reader &reader, glm::vec4 &value) {
            return reader.value(value.x) && reader.value(value.y) && reader.value(value.z) && reader.value(value.w) &&
                   finiteVec4(value);
        }

        void writeShader(Writer &writer, const ShaderSettings &shader) {
            const auto &palette = shader.palette;
            writer.value(static_cast<uint64_t>(palette.colors.size()));
            for (const glm::vec4 &color : palette.colors)
                writeVec4(writer, color);
            writer.enumeration(palette.iterationColoring);
            writer.enumeration(palette.singleIterationColoring);
            writer.value(palette.iterationInterval);
            writer.value(palette.offsetRatio);
            writer.value(palette.animationSpeed);

            const auto &stripe = shader.stripe;
            writer.enumeration(stripe.stripeType);
            writer.value(stripe.firstInterval);
            writer.value(stripe.secondInterval);
            writer.value(stripe.opacity);
            writer.value(stripe.offset);
            writer.value(stripe.animationSpeed);
            writer.enumeration(stripe.iterationColoring);

            const auto &slope = shader.slope;
            writer.value(slope.depth);
            writer.value(slope.reflectionRatio);
            writer.value(slope.opacity);
            writer.value(slope.zenith);
            writer.value(slope.azimuth);

            const auto &color = shader.color;
            writer.value(color.gamma);
            writer.value(color.exposure);
            writer.value(color.hue);
            writer.value(color.saturation);
            writer.value(color.brightness);
            writer.value(color.contrast);

            writer.value(shader.fog.radius);
            writer.value(shader.fog.opacity);
            writer.value(shader.bloom.threshold);
            writer.value(shader.bloom.radius);
            writer.value(shader.bloom.softness);
            writer.value(shader.bloom.intensity);

            writer.boolean(shader.sampling.bilinear);
            writer.value(shader.sampling.sampleCount);

            const auto &fractal3D = shader.fractal3D;
            writer.boolean(fractal3D.use);
            writer.value(fractal3D.altitude);
            writer.value(fractal3D.rotation);
            writer.value(fractal3D.distance);
            writer.value(fractal3D.baseIteration);
            writer.value(fractal3D.depthDivisor);
        }

        bool readShader(Reader &reader, ShaderSettings &shader) {
            auto &palette = shader.palette;
            uint64_t colorCount = 0;
            if (!reader.value(colorCount) || colorCount == 0 || colorCount > MAX_PALETTE_COLORS)
                return false;
            palette.colors.resize(static_cast<size_t>(colorCount));
            for (glm::vec4 &color : palette.colors) {
                if (!readVec4(reader, color))
                    return false;
            }
            if (!reader.enumeration(palette.iterationColoring, 0, 2) ||
                !reader.enumeration(palette.singleIterationColoring, 0, 2) ||
                !reader.value(palette.iterationInterval) || !reader.value(palette.offsetRatio) ||
                !reader.value(palette.animationSpeed))
                return false;

            auto &stripe = shader.stripe;
            if (!reader.enumeration(stripe.stripeType, 0, 3) || !reader.value(stripe.firstInterval) ||
                !reader.value(stripe.secondInterval) || !reader.value(stripe.opacity) ||
                !reader.value(stripe.offset) || !reader.value(stripe.animationSpeed) ||
                !reader.enumeration(stripe.iterationColoring, 0, 2))
                return false;

            auto &slope = shader.slope;
            if (!reader.value(slope.depth) || !reader.value(slope.reflectionRatio) || !reader.value(slope.opacity) ||
                !reader.value(slope.zenith) || !reader.value(slope.azimuth))
                return false;

            auto &color = shader.color;
            if (!reader.value(color.gamma) || !reader.value(color.exposure) || !reader.value(color.hue) ||
                !reader.value(color.saturation) || !reader.value(color.brightness) ||
                !reader.value(color.contrast) || !reader.value(shader.fog.radius) ||
                !reader.value(shader.fog.opacity) || !reader.value(shader.bloom.threshold) ||
                !reader.value(shader.bloom.radius) || !reader.value(shader.bloom.softness) ||
                !reader.value(shader.bloom.intensity) || !reader.boolean(shader.sampling.bilinear) ||
                !reader.value(shader.sampling.sampleCount) || !reader.boolean(shader.fractal3D.use) ||
                !reader.value(shader.fractal3D.altitude) || !reader.value(shader.fractal3D.rotation) ||
                !reader.value(shader.fractal3D.distance) || !reader.value(shader.fractal3D.baseIteration) ||
                !reader.value(shader.fractal3D.depthDivisor))
                return false;

            const auto allFinite = [](const std::initializer_list<float> values) {
                return std::ranges::all_of(values, [](const float value) { return finite(value); });
            };
            return palette.iterationInterval > 0.0f && shader.sampling.sampleCount > 0 &&
                   shader.sampling.sampleCount <= 4096 && shader.fractal3D.depthDivisor > 0.0f &&
                   allFinite({palette.iterationInterval, palette.offsetRatio, palette.animationSpeed,
                              stripe.firstInterval, stripe.secondInterval, stripe.opacity, stripe.offset,
                              stripe.animationSpeed, slope.depth, slope.reflectionRatio, slope.opacity, slope.zenith,
                              slope.azimuth, color.gamma, color.exposure, color.hue, color.saturation,
                              color.brightness, color.contrast, shader.fog.radius, shader.fog.opacity,
                              shader.bloom.threshold, shader.bloom.radius, shader.bloom.softness,
                              shader.bloom.intensity, shader.fractal3D.altitude, shader.fractal3D.rotation,
                              shader.fractal3D.distance, shader.fractal3D.baseIteration,
                              shader.fractal3D.depthDivisor});
        }

        void writeConfigPayload(Writer &writer, const Settings &settings, const uint32_t width,
                                const uint32_t height) {
            const auto &fractal = settings.fractal;
            writer.text(fractal.reference.center.clone_real().to_string());
            writer.text(fractal.reference.center.clone_imag().to_string());
            writer.value(fractal.general.bailout);
            writer.value(fractal.general.logZoom);
            writer.value(fractal.general.threads);

            writer.boolean(fractal.reference.useParallelRefCalculation);
            writer.value(fractal.reference.sync.referenceSynchronizationInterval);
            writer.value(fractal.reference.sync.referenceSynchronizationRadiusPower);
            writer.value(fractal.reference.compression.compressCriteria);
            writer.value(fractal.reference.compression.compressionThresholdPower);
            writer.boolean(fractal.reference.reuse);
            writer.boolean(fractal.reference.useFixedPrecision);
            writer.value(fractal.reference.fixedPrecisionNeg);

            writer.boolean(fractal.sa.use);
            writer.value(fractal.sa.appliedTermsCount);
            writer.value(fractal.sa.validatedTermsCount);
            writer.value(fractal.sa.epsilonPower);

            writer.value(fractal.mpa.minSkipReference);
            writer.value(fractal.mpa.maxMultiplierBetweenLevel);
            writer.value(fractal.mpa.epsilonPower);
            writer.enumeration(fractal.mpa.selectionMethod);
            writer.boolean(fractal.mpa.useCompress);
            writer.boolean(fractal.mpa.useParallelization);

            writer.value(fractal.perturb.maxIteration);
            writer.enumeration(fractal.perturb.decimalizeIterationMethod);
            writer.boolean(fractal.perturb.autoMaxIteration);
            writer.value(fractal.perturb.interiorDetectRadiusPower);
            writer.value(fractal.perturb.autoIterationMultiplier);
            writer.boolean(fractal.perturb.absoluteIterationMode);

            const auto &display = settings.render.display;
            writer.value(display.clarityMultiplier);
            writer.value(display.fps);
            writer.enumeration(display.pixelRenderPriority);

            const auto &compute = settings.render.computeShader;
            writer.boolean(compute.use);
            writer.value(compute.preferredBatchDuration);
            writer.value(compute.allowedGlitchPixelCount);
            writer.boolean(compute.completelyIgnoreMpa);
            writer.value(compute.automaticAcceptMpaBatches);
            writer.boolean(compute.interpolateIsolated);

            writer.value(width);
            writer.value(height);
            writeShader(writer, settings.shader);

            const auto &video = settings.video;
            writer.value(video.data.defaultZoomIncrement);
            writer.boolean(video.data.isStatic);
            writer.value(video.animation.overZoom);
            writer.boolean(video.animation.showText);
            writer.value(video.animation.mps);
            writer.value(video.exportation.fps);
            writer.value(video.exportation.bitrate);

            writer.boolean(settings.explore.autoMoveCursorToCenter);
            writer.value(settings.explore.autoAimRadiusPixels);
            writer.boolean(settings.explore.locator.burst);
            writer.boolean(settings.file.autoSaveBackup);
        }

        bool readConfigPayload(Reader &reader, Settings &settings, uint32_t &width, uint32_t &height,
                               const uint32_t version) {
            std::string real;
            std::string imag;
            if (!reader.text(real) || !reader.text(imag))
                return false;

            auto &fractal = settings.fractal;
            if (!reader.value(fractal.general.bailout) || !reader.value(fractal.general.logZoom) ||
                !reader.value(fractal.general.threads) ||
                !reader.boolean(fractal.reference.useParallelRefCalculation) ||
                !reader.value(fractal.reference.sync.referenceSynchronizationInterval) ||
                !reader.value(fractal.reference.sync.referenceSynchronizationRadiusPower) ||
                !reader.value(fractal.reference.compression.compressCriteria) ||
                !reader.value(fractal.reference.compression.compressionThresholdPower))
                return false;
            if (version == 1) {
                uint32_t obsoletePeriodMultiplier = 0;
                if (!reader.value(obsoletePeriodMultiplier))
                    return false;
            }
            if (!reader.boolean(fractal.reference.reuse) ||
                (version >= 2 && (!reader.boolean(fractal.reference.useFixedPrecision) ||
                                  !reader.value(fractal.reference.fixedPrecisionNeg))) ||
                !reader.boolean(fractal.sa.use) || !reader.value(fractal.sa.appliedTermsCount) ||
                !reader.value(fractal.sa.validatedTermsCount) || !reader.value(fractal.sa.epsilonPower) ||
                !reader.value(fractal.mpa.minSkipReference) ||
                !reader.value(fractal.mpa.maxMultiplierBetweenLevel) || !reader.value(fractal.mpa.epsilonPower) ||
                !reader.enumeration(fractal.mpa.selectionMethod, 0, 1) ||
                !reader.boolean(fractal.mpa.useCompress) || !reader.boolean(fractal.mpa.useParallelization) ||
                !reader.value(fractal.perturb.maxIteration) ||
                !reader.enumeration(fractal.perturb.decimalizeIterationMethod, 0, 4) ||
                !reader.boolean(fractal.perturb.autoMaxIteration) ||
                !reader.value(fractal.perturb.interiorDetectRadiusPower) ||
                !reader.value(fractal.perturb.autoIterationMultiplier) ||
                !reader.boolean(fractal.perturb.absoluteIterationMode))
                return false;

            auto &display = settings.render.display;
            auto &compute = settings.render.computeShader;
            if (!reader.value(display.clarityMultiplier) || !reader.value(display.fps) ||
                !reader.enumeration(display.pixelRenderPriority, 0, 1) || !reader.boolean(compute.use) ||
                !reader.value(compute.preferredBatchDuration) || !reader.value(compute.allowedGlitchPixelCount) ||
                !reader.boolean(compute.completelyIgnoreMpa) ||
                !reader.value(compute.automaticAcceptMpaBatches) ||
                !reader.boolean(compute.interpolateIsolated) || !reader.value(width) || !reader.value(height) ||
                !readShader(reader, settings.shader))
                return false;

            auto &video = settings.video;
            if (!reader.value(video.data.defaultZoomIncrement) || !reader.boolean(video.data.isStatic) ||
                !reader.value(video.animation.overZoom) || !reader.boolean(video.animation.showText) ||
                !reader.value(video.animation.mps) || !reader.value(video.exportation.fps) ||
                !reader.value(video.exportation.bitrate) ||
                !reader.boolean(settings.explore.autoMoveCursorToCenter) ||
                !reader.value(settings.explore.autoAimRadiusPixels) ||
                (version >= 2 && (!reader.boolean(settings.explore.locator.burst) ||
                                  !reader.boolean(settings.file.autoSaveBackup))))
                return false;

            const auto allFinite = [](const std::initializer_list<double> values) {
                return std::ranges::all_of(values, [](const double value) { return finite(value); });
            };
            if (!allFinite({fractal.general.bailout, fractal.general.logZoom, fractal.sa.epsilonPower,
                            fractal.mpa.epsilonPower, display.clarityMultiplier, display.fps,
                            compute.preferredBatchDuration, video.data.defaultZoomIncrement,
                            video.animation.overZoom, video.animation.mps, video.exportation.fps}) ||
                fractal.general.bailout < 2.0f || fractal.general.logZoom < 1.0f ||
                fractal.general.threads == 0 || fractal.perturb.maxIteration == 0 ||
                display.clarityMultiplier <= 0.0f || display.fps <= 0.0f ||
                compute.preferredBatchDuration <= 0.0f || width < 100 || height < 100 ||
                width > 16384 || height > 16384 ||
                video.data.defaultZoomIncrement <= 1.0f || video.animation.mps <= 0.0f ||
                video.exportation.fps <= 0.0f || video.exportation.bitrate == 0 ||
                settings.explore.autoAimRadiusPixels < 0)
                return false;

            if (const uint32_t hardwareThreads = std::thread::hardware_concurrency();
                hardwareThreads > 0 && fractal.general.threads > hardwareThreads)
                fractal.general.threads = hardwareThreads;

            if (!validDecimalText(real) || !validDecimalText(imag))
                return false;

            try {
                fractal.reference.center = fixed_point_complex(
                        real, imag, Perturbator::logZoomToExp10(fractal.general.logZoom));
            } catch (...) {
                return false;
            }
            return true;
        }

        bool readHeader(Reader &reader, const uint32_t expectedMagic, uint32_t &version) {
            uint32_t magic = 0;
            return reader.value(magic) && reader.value(version) && magic == expectedMagic && version >= 1 &&
                   version <= RFFSettingsIO::VERSION;
        }
    } // namespace

    bool RFFSettingsIO::saveConfig(const std::filesystem::path &path, const Settings &settings,
                                   const uint32_t width, const uint32_t height) {
        Writer writer(path);
        if (!writer.good())
            return false;
        writer.value(CONFIG_MAGIC);
        writer.value(VERSION);
        writeConfigPayload(writer, settings, width, height);
        return writer.finish();
    }

    bool RFFSettingsIO::loadConfig(const std::filesystem::path &path, Settings &settings,
                                   uint32_t *width, uint32_t *height) {
        Reader reader(path);
        uint32_t version = 0;
        if (!reader.good() || !readHeader(reader, CONFIG_MAGIC, version))
            return false;
        Settings candidate = settings;
        uint32_t candidateWidth = 0;
        uint32_t candidateHeight = 0;
        if (!readConfigPayload(reader, candidate, candidateWidth, candidateHeight, version))
            return false;
        settings = std::move(candidate);
        if (width)
            *width = candidateWidth;
        if (height)
            *height = candidateHeight;
        return true;
    }

    bool RFFSettingsIO::saveShaderPreset(const std::filesystem::path &path, const ShaderSettings &shader) {
        Writer writer(path);
        if (!writer.good())
            return false;
        writer.value(SHADER_MAGIC);
        writer.value(VERSION);
        writeShader(writer, shader);
        return writer.finish();
    }

    bool RFFSettingsIO::loadShaderPreset(const std::filesystem::path &path, ShaderSettings &shader) {
        Reader reader(path);
        uint32_t version = 0;
        if (!reader.good() || !readHeader(reader, SHADER_MAGIC, version))
            return false;
        ShaderSettings candidate = shader;
        if (!readShader(reader, candidate))
            return false;
        shader = std::move(candidate);
        return true;
    }
} // namespace merutilm::rff2
