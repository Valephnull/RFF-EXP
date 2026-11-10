#version 450
#define DESC_GRID 1
#include <desc_grid.glsl>

layout (set = 0, binding = 0) uniform sampler2D source_image;

layout (location = 0) out vec4 color;

void main() {
    color = texelFetch(source_image, ivec2(gl_FragCoord.xy), 0);
    color = applyGrid(color, gl_FragCoord.xy, false);
}
