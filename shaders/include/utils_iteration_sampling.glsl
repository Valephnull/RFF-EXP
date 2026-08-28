#ifndef UTILS_ITERATION_SAMPLING_INCLUDE
#define UTILS_ITERATION_SAMPLING_INCLUDE

#include <utils_iteration_filter.glsl>
#include <utils_stochastic.glsl>

double bilinear_iteration(vec2 coordinate) {
    vec2 centered = coordinate - vec2(0.5);
    ivec2 base = ivec2(floor(centered));
    vec2 fraction = fract(centered);

    double i00 = get_iteration(base);
    double i10 = get_iteration(base, ivec2(1, 0));
    double i01 = get_iteration(base, ivec2(0, 1));
    double i11 = get_iteration(base, ivec2(1, 1));
    return filtered_bilinear_iteration(dvec4(i00, i10, i01, i11), fraction);
}

double sample_iteration(vec2 coordinate) {
    if (sampling_settings.bilinear)
        return bilinear_iteration(coordinate);
    double iteration = get_iteration(ivec2(coordinate));
    return valid_iteration_sample(iteration) ? iteration : 0.0;
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
