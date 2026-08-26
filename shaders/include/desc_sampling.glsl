#ifndef DESC_SAMPLING_INCLUDE
#define DESC_SAMPLING_INCLUDE

layout (set = DESC_SAMPLING, binding = 0) uniform SamplingUBO {
    bool bilinear;
    uint sample_count;
} sampling_settings;

#endif
