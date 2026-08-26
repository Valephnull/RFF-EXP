#ifndef UTILS_STOCHASTIC_INCLUDE
#define UTILS_STOCHASTIC_INCLUDE

const uint MAX_STOCHASTIC_SAMPLES = 256u;

uint sampling_hash(uint value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

float sampling_random(uint value) {
    return float(sampling_hash(value)) * (1.0 / 4294967296.0);
}

vec2 stochastic_pixel_offset(ivec2 pixel, uint sample_index) {
    uint seed = uint(pixel.x) * 0x1f123bb5u ^ uint(pixel.y) * 0x5f356495u ^
                sample_index * 0x9e3779b9u;
    return vec2(sampling_random(seed), sampling_random(seed ^ 0x68bc21ebu)) - 0.5;
}

#endif
