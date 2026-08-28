#include <bit>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "rff2/io/RFFDynamicMapBinary.h"
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
}

int main() {
    const RFFDynamicMapBinary valid(42.0f, 7, 100, {1.0, 2.5, 100.0, 4.0}, 2, 2);
    if (!expect(valid.hasValidIterations(), "A complete finite map was rejected"))
        return 1;
    const auto decoded = RFFDynamicMapBinary::decode(valid.encode());
    if (!expect(decoded.hasValidIterations(), "A valid encoded map did not round-trip"))
        return 2;

    const RFFDynamicMapBinary interrupted(42.0f, 7, 100, {1.0, 0.0, 3.0, 4.0}, 2, 2);
    if (!expect(interrupted.hasData() && !interrupted.hasValidIterations(),
                "An interrupted-compute zero was not detected"))
        return 3;

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
