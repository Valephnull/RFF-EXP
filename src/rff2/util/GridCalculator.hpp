#pragma once

#include <algorithm>
#include <bit>
#include <climits>
#include <cmath>
#include <cstring>
#include <limits>
#include <glm/glm.hpp>
#include "../calc/fixed_point_complex.hpp"
#include "../settings/ShdGridSettings.hpp"

namespace merutilm::rff2 {
    struct GridUniform {
        glm::vec4 color{};
        glm::vec4 geometry{}; // Coarse spacing, subdivision, fine opacity, line width (render pixels).
        glm::vec4 placement{}; // Center's fractional cell coordinates, render width and height.
        glm::vec4 stage{}; // x: 1 for the live post-zoom pass, 0 for the video pass.
    };
    static_assert(sizeof(GridUniform) == 64);

    // Reduce the arbitrary-precision center modulo a cell before sending it to
    // the GPU. Converting the full coordinate to double would lose the grid at deep zooms.
    class GridCalculator {
        struct Integer {
            mpz_t value;
            Integer() { mpz_init(value); }
            ~Integer() { mpz_clear(value); }
            Integer(const Integer &) = delete;
            Integer &operator=(const Integer &) = delete;
        } power;
        fixed_point_complex cachedCenter;
        int64_t cachedLevel = INT64_MIN;
        uint32_t cachedSubdivision = 0;
        glm::dvec2 cachedPhase{};

        static bool equal(const fixed_point_decimal &a, const fixed_point_decimal &b) {
            return a.size == b.size && a.exp2div64 == b.exp2div64 &&
                   (a.size == 0 || std::memcmp(a.data, b.data, static_cast<size_t>(std::abs(a.size)) * sizeof(mp_limb_t)) == 0);
        }

        double phase(const fixed_point_decimal &coordinate, const int64_t level) const {
            if (coordinate.size == 0) return 0;
            Integer numerator, denominator;
            mpz_import(numerator.value, static_cast<size_t>(std::abs(coordinate.size)), -1,
                       sizeof(mp_limb_t), 0, 0, coordinate.data);
            if (coordinate.size < 0) mpz_neg(numerator.value, numerator.value);
            mpz_set_ui(denominator.value, 1);
            if (coordinate.exp2div64 < 0)
                mpz_mul_2exp(denominator.value, denominator.value, static_cast<mp_bitcnt_t>(-coordinate.exp2div64) * 64);
            else
                mpz_mul_2exp(numerator.value, numerator.value, static_cast<mp_bitcnt_t>(coordinate.exp2div64) * 64);
            if (level >= 0) mpz_mul(numerator.value, numerator.value, power.value);
            else mpz_mul(denominator.value, denominator.value, power.value);
            mpz_mod(numerator.value, numerator.value, denominator.value);
            long ne = 0, de = 0;
            const double n = mpz_get_d_2exp(&ne, numerator.value);
            const double d = mpz_get_d_2exp(&de, denominator.value);
            return ne - de < -1074 ? 0 : std::ldexp(n / d, static_cast<int>(ne - de));
        }

    public:
        GridUniform calculate(const ShdGridSettings &settings, const fixed_point_complex &center,
                              const double renderLogZoom, const double visibleLogZoom, const float clarity,
                              const glm::uvec2 extent, const glm::dvec2 centerOffsetPixels = {}) {
            GridUniform result{};
            if (!settings.use || settings.opacity <= 0 || clarity <= 0 || extent.x == 0 || extent.y == 0)
                return result;
            const auto subdivisions = std::clamp(settings.subdivision, 2u, 64u);
            const double logBase = std::log10(static_cast<double>(subdivisions));
            const double levelPosition = (visibleLogZoom - std::log10(std::clamp(settings.spacing, 16.0f, 2048.0f))) / logBase;
            if ((std::bit_cast<uint64_t>(levelPosition) & UINT64_C(0x7ff0000000000000)) == UINT64_C(0x7ff0000000000000) ||
                std::abs(levelPosition) > static_cast<double>(ULONG_MAX) - 1)
                return result;
            const auto level = static_cast<int64_t>(std::floor(levelPosition));
            const bool newScale = level != cachedLevel || subdivisions != cachedSubdivision;
            if (newScale) {
                mpz_ui_pow_ui(power.value, subdivisions, static_cast<unsigned long>(std::abs(level)));
                cachedLevel = level;
                cachedSubdivision = subdivisions;
            }
            if (newScale || !equal(center.real, cachedCenter.real) || !equal(center.imag, cachedCenter.imag)) {
                cachedCenter = center;
                cachedPhase = {phase(center.real, level), phase(center.imag, level)};
            }
            const double spacing = std::pow(10.0, renderLogZoom - static_cast<double>(level) * logBase) * clarity;
            const double zoomScale = std::pow(10.0, visibleLogZoom - renderLogZoom);
            const double t = levelPosition - static_cast<double>(level);
            const auto offset = cachedPhase + centerOffsetPixels / spacing;
            result.color = glm::vec4(glm::vec3(settings.color), std::clamp(settings.opacity, 0.0f, 1.0f));
            result.geometry = {static_cast<float>(spacing), static_cast<float>(subdivisions),
                               static_cast<float>(t * t * (3 - 2 * t)),
                               static_cast<float>(std::clamp(settings.lineWidth, 0.5f, 8.0f) * clarity / zoomScale)};
            result.placement = {static_cast<float>(offset.x - std::floor(offset.x)),
                                static_cast<float>(offset.y - std::floor(offset.y)), extent.x, extent.y};
            return result;
        }
    };
}
