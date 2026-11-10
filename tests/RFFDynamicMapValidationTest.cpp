#include <bit>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "rff2/io/RFFDynamicMapBinary.hpp"
#include "rff2/io/RFFLocationBinary.hpp"
#include "rff2/renderpool/RenderPoolJob.hpp"
#include "rff2/app/IOUtilities.h"

using namespace merutilm::rff2;

// RFFDynamicMapBinary's file helpers reference these two naming functions.
// This unit test exercises only its in-memory encoding and validation, so keep
// the test independent of the native file-dialog library.
namespace merutilm::rff2 {
    std::string IOUtilities::fileNameFormat(const unsigned int n, const std::string_view extension) {
        return std::to_string(n) + "." + std::string(extension);
    }

    std::filesystem::path IOUtilities::generateFilename(const std::filesystem::path &dir,
                                                        const std::string_view extension, uint32_t *cnt) {
        if (cnt != nullptr)
            *cnt = 1;
        return dir / fileNameFormat(1, extension);
    }
} // namespace merutilm::rff2

namespace {
    bool expect(const bool condition, const char *message) {
        if (condition)
            return true;
        std::cerr << message << '\n';
        return false;
    }

    template<typename T>
    void append(std::vector<std::byte> &bytes, const T value) {
        const auto start = bytes.size();
        bytes.resize(start + sizeof(value));
        std::memcpy(bytes.data() + start, &value, sizeof(value));
    }

    bool compatibilityChecks() {
        const itt::ptb wide = (itt::ptb{1} << 90) + 123;
        const RFFDynamicMapBinary map(42.0, wide - 1, wide, {1.0, 2.5, 100.0, 4.0}, 2, 2);
        const auto bytes = map.encode();
        const auto decoded = RFFDynamicMapBinary::decode(bytes);
        if (!expect(decoded.period == wide - 1 && decoded.maxIteration == wide,
                    "Wide iteration counts were truncated in maps"))
            return false;
        auto upstream = bytes;
        const uint32_t upstreamVersion = 5;
        std::memcpy(upstream.data(), &upstreamVersion, sizeof(upstreamVersion));
        if (!expect(RFFDynamicMapBinary::decode(upstream).maxIteration == wide,
                    "Upstream v5 maps cannot be loaded"))
            return false;

        for (const uint32_t version : {4U, 5U}) {
            std::vector<std::byte> legacy;
            append(legacy, version);
            append(legacy, uint16_t{2});
            append(legacy, uint16_t{2});
            append(legacy, 42.0);
            append(legacy, uint64_t{7});
            append(legacy, uint64_t{100});
            for (const auto value : {1.0, 2.5, 100.0, 4.0})
                append(legacy, value);
            const auto old = RFFDynamicMapBinary::decode(legacy);
            if (!expect(old.hasValidIterations() && old.period == 7 && old.maxIteration == 100,
                        "Legacy RFF-EXP map cannot be loaded"))
                return false;
        }
        auto malformed = bytes;
        const uint64_t badCounterSize = UINT64_MAX;
        std::memcpy(malformed.data() + 16, &badCounterSize, sizeof(badCounterSize));
        if (!expect(!RFFDynamicMapBinary::decode(malformed).hasData(), "Invalid sized counter was accepted"))
            return false;

        const auto path = std::filesystem::temp_directory_path() / "rff_exp_location_compat.rfl";
        RFFLocationBinary(42.0, "-0.75", "0.1", wide).exportFile(path);
        if (!expect(RFFLocationBinary::read(path).getMaxIteration() == wide,
                    "Wide iteration count was truncated in locations"))
            return false;
        for (const uint32_t version : {4U, 5U}) {
            // 16 collides with upstream's counter-width prefix; test the fallback.
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            IOUtilities::encodeAndWrite(out, version);
            IOUtilities::encodeAndWrite(out, 42.0);
            IOUtilities::encodeAndWrite(out, uint64_t{16});
            IOUtilities::encodeAndWrite(out, uint64_t{5});
            IOUtilities::encodeAndWrite(out, "-0.75", 5);
            IOUtilities::encodeAndWrite(out, uint64_t{3});
            IOUtilities::encodeAndWrite(out, "0.1", 3);
            out.close();
            const auto location = RFFLocationBinary::read(path);
            if (!expect(location.hasData() && location.maxIteration == 16 && location.real == "-0.75",
                        "Legacy RFF-EXP location cannot be loaded"))
                return false;
        }
        std::filesystem::remove(path);

        RenderPoolJobManifest job;
        job.id = 1;
        job.windowWidth = 1280;
        job.windowHeight = 720;
        job.frameCount = 10;
        job.startLogZoom = 42;
        job.zoomIncrement = 2;
        job.clarityMultiplier = 1.75f;
        job.centerReal = "-0.75";
        job.centerImag = "0.1";
        job.maxIteration = wide;
        job.referencePartitionSize = 1024;
        job.simd = true;
        RenderPoolJobManifest restored;
        std::string error;
        if (!expect(RenderPoolJobManifest::decode(job.encode(), restored, error) && restored.maxIteration == wide &&
                    restored.referencePartitionSize == 1024 && restored.simd && restored.clarityMultiplier == 1.75f,
                    "Render-pool manifest did not preserve new settings"))
            return false;
        // Produce the previous wire format from a low-limit v2 manifest.
        job.maxIteration = 100;
        auto legacyJob = job.encode();
        legacyJob[1] = std::byte{1};
        legacyJob.erase(legacyJob.end() - 5, legacyJob.end());
        constexpr size_t tailAfterIteration = 11;
        const auto counterStart = legacyJob.end() - tailAfterIteration - sizeof(itt::ptb);
        legacyJob.erase(counterStart, counterStart + 8);
        return expect(RenderPoolJobManifest::decode(legacyJob, restored, error) && restored.maxIteration == 100,
                      "Legacy render-pool manifest cannot be loaded");
    }
}

int main() {
    if (!compatibilityChecks())
        return 20;
    const RFFDynamicMapBinary valid(42.0f, 7, 100, {1.0, 2.5, 100.0, 4.0}, 2, 2);
    if (!expect(valid.hasValidIterations(), "A complete finite map was rejected"))
        return 1;
    const auto decoded = RFFDynamicMapBinary::decode(valid.encode());
    if (!expect(decoded.hasValidIterations(), "A valid encoded map did not round-trip"))
        return 2;

    const RFFDynamicMapBinary validWithZero(42.0f, 7, 100, {1.0, 0.0, 3.0, 4.0}, 2, 2);
    if (!expect(validWithZero.hasValidIterations(), "A valid map containing a zero-valued pixel was rejected"))
        return 3;

    const RFFDynamicMapBinary uninitialized(42.0f, 7, 100, {0.0, 0.0, 0.0, 0.0}, 2, 2);
    if (!expect(uninitialized.hasData() && !uninitialized.hasValidIterations(),
                "An entirely uninitialized map was accepted"))
        return 8;

    const RFFDynamicMapBinary negative(42.0f, 7, 100, {1.0, -2.0, 3.0, 4.0}, 2, 2);
    if (!expect(!negative.hasValidIterations(), "A negative iteration was not detected"))
        return 4;

    const RFFDynamicMapBinary impossible(42.0f, 7, 100, {1.0, 101.0, 3.0, 4.0}, 2, 2);
    if (!expect(!impossible.hasValidIterations(), "An iteration above the keyframe limit was not detected"))
        return 5;

    const double infinity = std::bit_cast<double>(uint64_t{0x7ff0000000000000});
    const double nan = std::bit_cast<double>(uint64_t{0x7ff8000000000001});
    const RFFDynamicMapBinary nonFinite(42.0f, 7, 100, {1.0, infinity, nan, 4.0}, 2, 2);
    if (!expect(!nonFinite.hasValidIterations(), "Non-finite iterations were not detected under fast-math"))
        return 6;

    const RFFDynamicMapBinary noIterationLimit(42.0f, 7, 0, {1.0}, 1, 1);
    if (!expect(!noIterationLimit.hasData(), "A zero maximum-iteration header was accepted"))
        return 7;
    return 0;
}
