#version 450
#include <common.glsl>

// define descriptors
#define DESC_ITERATION 1
#define DESC_STRIPE 2
#define DESC_TIME 3
#define DESC_SAMPLING 4

// include descriptors
#include <desc_iteration.glsl>
#include <desc_stripe.glsl>
#include <desc_time.glsl>
#include <desc_sampling.glsl>

// include utilities
#include <utils_iteration.glsl>
#include <utils_stripe.glsl>
#include <utils_iteration_sampling.glsl>

layout (set = 0, binding = 0) uniform sampler2D canvas;

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexcoord;

layout (location = 0) out vec4 color;


void main() {

    ivec2 iter_coord = ivec2(gl_FragCoord.xy);
    vec2 center = vec2(iter_coord) + vec2(0.5);
    uint sample_count = clamp(sampling_settings.sample_count, 1u, MAX_STOCHASTIC_SAMPLES);
    float multiplier_total = 0.0;
    for (uint sample_index = 0u; sample_index < sample_count; ++sample_index) {
        vec2 jitter = sample_count > 1u ? stochastic_pixel_offset(iter_coord, sample_index) : vec2(0.0);
        multiplier_total += stripe_get_multiplier(sample_iteration(center + jitter));
    }
    float multiplier = multiplier_total / float(sample_count);

    color = vec4(texelFetch(canvas, ivec2(gl_FragCoord.xy), 0).rgb * multiplier, 1);
}
