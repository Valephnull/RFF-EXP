#include "rff2/app/VideoExportValidation.hpp"
#include "rff2/util/ZoomCounterText.hpp"

#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace merutilm::rff2;

int main() try {
    auto expect = [](bool condition, const std::source_location location = std::source_location::current()) {
        if (!condition) throw std::runtime_error("Video export validation failed at line " + std::to_string(location.line()));
    };
    expect(validateVideoExport(60, 1, 0, 2, 1280, 720).empty());
    VidAnimationSettings counter{.overZoom = 0, .showText = true, .mps = 1,
                                 .zoomCounterLabel = "Scale", .zoomCounterPrecision = 3};
    expect(formatZoomCounter(counter, 5.5) == "Scale : 3.162E5");
    counter.zoomCounterLabel.clear();
    counter.zoomCounterPrecision = 0;
    expect(formatZoomCounter(counter, 2.0) == "1E2");
    expect(!validateVideoExport(0, 1, 0, 2, 1280, 720).empty());
    expect(!validateVideoExport(60, 0, 0, 2, 1280, 720).empty());
    expect(!validateVideoExport(60, -1, 0, 2, 1280, 720).empty());
    expect(!validateVideoExport(60, 1, -1, 2, 1280, 720).empty());
    expect(!validateVideoExport(60, 1, 0, 1, 1280, 720).empty());
    expect(!validateVideoExport(60, 1, 0, 2, 0, 720).empty());
    expect(!validateVideoExport(60, 1, 0, 2, 1281, 720).empty());
    expect(!validateVideoExport(60, 1, 0, 2, 1280, 721).empty());
    for (uint64_t bits : {UINT64_C(0x7ff0000000000000), UINT64_C(0xfff0000000000000),
                         UINT64_C(0x7ff8000000000001)}) {
        volatile uint64_t runtimeBits = bits;
        const auto invalid = std::bit_cast<double>(static_cast<uint64_t>(runtimeBits));
        expect(!validateVideoExport(invalid, 1, 0, 2, 1280, 720).empty());
        expect(!validateVideoExport(60, invalid, 0, 2, 1280, 720).empty());
        expect(!validateVideoExport(60, 1, invalid, 2, 1280, 720).empty());
        expect(!validateVideoExport(60, 1, 0, invalid, 1280, 720).empty());
    }
    std::cout << "Video export validation passed\n";
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
