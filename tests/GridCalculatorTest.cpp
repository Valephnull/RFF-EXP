#include <cassert>
#include <cmath>
#include "rff2/util/GridCalculator.hpp"

using namespace merutilm::rff2;

int main() {
    GridCalculator calculator;
    ShdGridSettings settings;
    settings.use = true;
    settings.spacing = 100;
    const fixed_point_complex center("-0.743625", "0.131825", -100);
    auto near = [](double a, double b, double tolerance = 1e-5) { return std::abs(a - b) < tolerance; };
    auto frame = calculator.calculate(settings, center, 6, 6, 1, {1280,720});
    assert(near(frame.geometry.x, 100));
    assert(near(frame.placement.x, 0.75) && near(frame.placement.y, 0.25));
    assert(near(frame.geometry.z, 0));
    auto finer = calculator.calculate(settings, center, 6.5, 6.5, 1, {1280,720});
    assert(near(finer.geometry.z, 0.5));
    const auto twice = calculator.calculate(settings, center, 6.5, 6.5, 2, {2560,1440});
    assert(near(twice.geometry.x, 2 * finer.geometry.x));
    assert(near(twice.geometry.w, 2 * finer.geometry.w));
    assert(twice.placement.x == finer.placement.x);

    // A 10^-1000 offset must remain visible after a zoom far beyond double precision.
    const fixed_point_complex deep("-0.7500000000000000000000000000000000000000", "0", -1100);
    fixed_point_complex shifted = deep;
    fixed_point_complex delta(dex(-3322, 1.0), dex::ZERO, -1100);
    fixed_point_complex::add(shifted, shifted, delta);
    const auto deepA = calculator.calculate(settings, deep, 1002, 1002, 1, {1280,720});
    const auto deepB = calculator.calculate(settings, shifted, 1002, 1002, 1, {1280,720});
    assert(std::abs(deepA.placement.x - deepB.placement.x) > 0.01);

    for (uint32_t subdivision : {2u,4u,10u}) {
        settings.subdivision = subdivision;
        const double boundary = 2 + 8 * std::log10(static_cast<double>(subdivision));
        const auto before = calculator.calculate(settings, center, boundary - 1e-7, boundary - 1e-7, 1, {1280,720});
        const auto after = calculator.calculate(settings, center, boundary + 1e-7, boundary + 1e-7, 1, {1280,720});
        assert(before.geometry.z > 0.999 && after.geometry.z < 0.001);
        assert(near(before.geometry.x / subdivision, after.geometry.x, 1e-3));
        const double phase = before.placement.x * subdivision;
        const double difference = std::abs((phase - std::floor(phase)) - after.placement.x);
        assert(std::min(difference, 1 - difference) < 1e-5);
    }
    settings.use = false;
    assert(calculator.calculate(settings, center, 6, 6, 1, {1280,720}).color.a == 0);
}
