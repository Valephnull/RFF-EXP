#ifndef UTILS_ITERATION_SAMPLING_INCLUDE
#define UTILS_ITERATION_SAMPLING_INCLUDE

#include <utils_stochastic.glsl>

double bilinear_iteration(vec2 coordinate) {
    vec2 centered = coordinate - vec2(0.5);
    ivec2 base = ivec2(floor(centered));
    vec2 fraction = fract(centered);

    double i00 = get_iteration(base);
    double i10 = get_iteration(base, ivec2(1, 0));
    double i01 = get_iteration(base, ivec2(0, 1));
    double i11 = get_iteration(base, ivec2(1, 1));
    double lower = mix(i00, i10, double(fraction.x));
    double upper = mix(i01, i11, double(fraction.x));
    return mix(lower, upper, double(fraction.y));
}

double sample_iteration(vec2 coordinate) {
    if (sampling_settings.bilinear)
        return bilinear_iteration(coordinate);
    return get_iteration(ivec2(coordinate));
}

double sample_pixel_iteration(ivec2 pixel) {
    vec2 center = vec2(pixel) + vec2(0.5);
    uint count = clamp(sampling_settings.sample_count, 1u, MAX_STOCHASTIC_SAMPLES);
    double total = 0.0;
    for (uint sample_index = 0u; sample_index < count; ++sample_index)
        total += sample_iteration(center + (count > 1u ? stochastic_pixel_offset(pixel, sample_index) : vec2(0.0)));
    return total / double(count);
}

#endif
