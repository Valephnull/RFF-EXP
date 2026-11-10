#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

#include "rff2/io/GradientLoader.hpp"

using namespace merutilm::rff2;

namespace {
    void expect(const bool condition, const char *message) {
        if (!condition) throw std::runtime_error(message);
    }

    bool near(const float a, const float b) { return std::abs(a - b) < 1e-5f; }

    void checkPack(const std::vector<ImportedGradient> &pack) {
        expect(!pack.empty(), "No gradients were parsed");
        for (const auto &gradient : pack) {
            expect(!gradient.name.empty(), "Gradient has no name");
            expect(gradient.colors.size() == 400, "Incorrect palette length");
            for (const auto &color : gradient.colors) {
                for (int i = 0; i < 4; ++i)
                    expect(std::isfinite(color[i]) && color[i] >= 0 && color[i] <= 1, "Invalid sampled color");
                expect(color.a == 1, "Unexpected transparency in color palette");
            }
        }
    }
}

int main(const int argc, char **argv) {
    try {
        std::string error;
        auto pack = GradientLoader::parse(R"(
; Braces and field names in comments are ignored: { index=900 }
First {
 gradient: title="Synthetic RGB" smooth=no numnodes=2
 index=0 color=255
 index=200 color=16711680
 opacity: smooth=no index=0 opacity=128
}
Second { gradient: title="Quoted \"title\" { }" smooth=yes
 index=0 color=0 index=100 color=65280 index=399 color=16777215
}
)", error);
        expect(error.empty() && pack.size() == 2, "Multi-gradient parsing failed");
        checkPack(pack);
        const auto &linear = pack[0].colors;
        expect(pack[0].name == "Synthetic RGB", "Title was not preserved");
        expect(linear[0].r == 1 && linear[0].g == 0 && linear[0].b == 0, "COLORREF red channel was decoded incorrectly");
        expect(linear[200].b == 1 && linear[200].r == 0, "COLORREF blue channel was decoded incorrectly");
        expect(near(linear[100].r, 0.5f) && near(linear[100].b, 0.5f), "Stop positions were not interpolated correctly");
        expect(near(linear[300].r, 0.5f) && near(linear[300].b, 0.5f), "Palette wrap interpolation failed");
        expect(pack[1].name == "Quoted \"title\" { }", "Quoted title or braces were parsed incorrectly");
        expect(pack[1].colors[100].g == 1 && pack[1].colors[100].r == 0, "Smooth curve lost a control point");

        pack = GradientLoader::parse("\xef\xbb\xbfSolid { gradient: index=12 color=0x123456 }", error);
        expect(error.empty() && pack.size() == 1 && pack[0].name == "Solid", "BOM, hex color, or fallback name failed");
        checkPack(pack);
        expect(near(pack[0].colors[0].r, 0x56 / 255.0f) && near(pack[0].colors[399].b, 0x12 / 255.0f),
               "Solid palette changed its color");

        pack = GradientLoader::parse("Uneven { gradient: index=300 color=65280 index=0 color=0 index=100 color=255 }", error);
        expect(error.empty() && pack.size() == 1, "Unsorted stops failed");
        expect(near(pack[0].colors[200].r, 0.5f) && near(pack[0].colors[200].g, 0.5f), "Uneven stop spacing was lost");

        pack = GradientLoader::parse("Rotate { gradient: rotation=-25 index=0 color=255 index=200 color=16711680 }", error);
        expect(error.empty() && pack.size() == 1 && pack[0].colors[375].r == 1, "Negative rotation failed");
        pack = GradientLoader::parse("Duplicate { gradient: index=0 color=255 index=0 color=65280 }", error);
        expect(error.empty() && pack[0].colors[0].g == 1 && pack[0].colors[0].r == 0, "Duplicate stop did not use the last color");

        for (const std::string text : {
                "", "Broken { gradient: index=0 color=0", "Broken { gradient: index=400 color=0 }",
                "Broken { gradient: index=-1 color=0 }", "Broken { gradient: index=0 color=16777216 }",
                "Broken { gradient: index=0 color=-1 }", "Broken { gradient: index=0 color=99999999999999999999999999 }",
                "Broken { gradient: index=0 }", "Broken { gradient: color=255 }", "Broken { gradient: }",
                "Broken { gradient: smooth=maybe index=0 color=0 }", "Broken { gradient: title=\"unfinished",
                "Broken { nested { gradient: index=0 color=0 } }", "}"}) {
            const auto invalid = GradientLoader::parse(text, error);
            expect(invalid.empty() && !error.empty(), "Malformed gradient was accepted");
        }
        pack = GradientLoader::parse("Good { gradient: index=0 color=0 } Bad { gradient: color=255 }", error);
        expect(pack.empty() && !error.empty(), "A partially malformed pack was accepted");

        if (argc > 1) {
            // Optional external reference check; no user-supplied gradients are stored in the repository.
            pack = GradientLoader::load(std::filesystem::path(argv[1]), error);
            if (!error.empty()) throw std::runtime_error(error);
            checkPack(pack);
            std::cout << "Validated " << pack.size() << " gradients from the external reference file.\n";
        }
        std::cout << "Gradient import tests passed.\n";
        return 0;
    } catch (const std::exception &exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
