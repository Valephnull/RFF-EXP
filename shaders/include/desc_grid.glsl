#ifndef DESC_GRID_GLSL
#define DESC_GRID_GLSL
layout (set = DESC_GRID, binding = 0) uniform GridUBO {
    vec4 color;
    vec4 geometry;
    vec4 placement;
    vec4 stage;
} grid_settings;

float gridLine(vec2 cell, float spacing, float width) {
    vec2 distancePx = abs(fract(cell + 0.5) - 0.5) * spacing;
    float distanceToLine = min(distancePx.x, distancePx.y);
    return 1.0 - smoothstep(max(0.0, width * 0.5 - 0.5), width * 0.5 + 0.5, distanceToLine);
}

vec4 applyGrid(vec4 original, vec2 pixel, bool afterZoom) {
    if (grid_settings.color.a <= 0.0 || (grid_settings.stage.x > 0.5) != afterZoom) return original;
    float spacing = grid_settings.geometry.x;
    float subdivision = grid_settings.geometry.y;
    vec2 offset = (pixel - grid_settings.placement.zw * 0.5) * vec2(1.0, -1.0);
    vec2 cell = offset / spacing + grid_settings.placement.xy;
    float coarse = gridLine(cell, spacing, grid_settings.geometry.w);
    float fine = gridLine(cell * subdivision, spacing / subdivision, grid_settings.geometry.w);
    float coverage = max(coarse, fine * grid_settings.geometry.z);
    return vec4(mix(original.rgb, grid_settings.color.rgb, coverage * grid_settings.color.a), original.a);
}
#endif
