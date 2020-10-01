#version 430 core

#define saturate(x) clamp(x, 0.0, 1.0)
#define epsilon 0.005
#define rgb_to_luminance vec3(0.2125, 0.7154, 0.0721)

layout(local_size_x = 32, local_size_y = 32) in;

layout(binding = 0, rgba32f) uniform readonly image2D hdr_color;
layout(std430, binding = 1) buffer luminance_data
{
    uint histogram[256];
    float luminance;
};

layout(location = 1) uniform vec2 params; // min_log_luminance (x), inverse_log_luminance_range (y)

shared uint shared_histogram[256];

uint color_to_luminance_bin(in vec3 pixel_color, in float min_log_luminance, in float inverse_log_luminance_range);

void main()
{
    shared_histogram[gl_LocalInvocationIndex] = 0;

    groupMemoryBarrier();
    barrier();

    ivec2 dim = imageSize(hdr_color);
    if (gl_GlobalInvocationID.x < dim.x && gl_GlobalInvocationID.y < dim.y)
    {
        uint weight = 1;
        vec3 pixel_color = imageLoad(hdr_color, ivec2(gl_GlobalInvocationID.xy)).rgb;
        uint bin_idx = color_to_luminance_bin(pixel_color, params.x, params.y);
        atomicAdd(shared_histogram[bin_idx], weight);
    }

    groupMemoryBarrier();
    barrier();

    atomicAdd(histogram[gl_LocalInvocationIndex], shared_histogram[gl_LocalInvocationIndex]);
}

uint color_to_luminance_bin(in vec3 pixel_color, in float min_log_luminance, in float inverse_log_luminance_range)
{
    float lum = dot(pixel_color, rgb_to_luminance);

    if(lum < epsilon)
        return 0;

    float luminance_log = saturate((log2(lum) - min_log_luminance) * inverse_log_luminance_range);

    return uint(luminance_log * 254.0 + 1.0);
}