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
    return float(sampling_hash(value) & 0x00ffffffu) * (1.0 / 16777216.0);
}

vec2 stochastic_pixel_offset(ivec2 pixel, uint sample_index) {
    uint seed = sampling_hash(uint(pixel.x) * 0x9e3779b9u ^
                              uint(pixel.y) * 0x85ebca6bu ^ 0xc2b2ae35u);
    vec2 rotation = vec2(sampling_random(seed ^ 0x68bc21ebu),
                         sampling_random(seed ^ 0x02e5be93u));
    vec2 sequence = (float(sample_index) + 0.5) * vec2(0.7548776662, 0.5698402910);
    return fract(rotation + sequence) - 0.5;
}

#endif
