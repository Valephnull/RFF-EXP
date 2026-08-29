#version 450
#include <common.glsl>

// define descriptors
#define DESC_ITERATION 0
#define DESC_PALETTE 1
#define DESC_TIME 2
#define DESC_BATCH_RESULT 3
#define DESC_SMOOTH_ZOOM 4
#define DESC_SAMPLING 5

// include descriptors
#include <desc_iteration.glsl>
#include <desc_palette.glsl>
#include <desc_time.glsl>
#include <desc_batch_result.glsl>
#include <desc_smooth_zoom.glsl>
#include <desc_sampling.glsl>

// include utilities
#include <utils_iteration.glsl>
#include <utils_palette.glsl>
#include <utils_iteration_sampling.glsl>


layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexcoord;

layout (location = 0) out vec4 color;
layout (constant_id = 0) const uint perturbation_main_iterator = 0;

void main() {

    ivec2 iter_coord = ivec2(gl_FragCoord.xy);
    vec2 center = vec2(iter_coord) + vec2(0.5);
    uint sample_count = clamp(sampling_settings.sample_count, 1u, MAX_STOCHASTIC_SAMPLES);
    vec4 color_total = vec4(0.0);
    for (uint sample_index = 0u; sample_index < sample_count; ++sample_index) {
        vec2 jitter = sample_count > 1u ? stochastic_pixel_offset(iter_coord, sample_index) : vec2(0.0);
        double iteration = sample_iteration(center + jitter);
        color_total += iteration == 0.0 ? vec4(0, 0, 0, 1) : palette_get_color(iteration);
    }
    color = color_total / float(sample_count);

    uint index = (iteration_info_settings.extent.y - iter_coord.y - 1) * iteration_info_settings.extent.x + iter_coord.x;
    if(smooth_zoom_settings.pos_delta == vec2(0) && smooth_zoom_settings.log_zoom_delta == 0 && perturbation_main_iterator == PMI_GPU && batch_result_data.completed[index] != 1){
        color.rgb *= 0.2;
    }

}
