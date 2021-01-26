#define COMPUTE
#include <../include/common_constants_and_functions.glsl>

layout(local_size_x = 16, local_size_y = 16) in;

layout(std430, binding = 6) buffer luminance_data
{
    uint histogram[256];
    float luminance;
};

layout(location = 0) uniform vec4 params; // time coefficient (x), pixel_count (y), min_log_luminance (z), log_luminance_range (w)
#define time_coefficient params.x
#define pixel_count params.y
#define min_log_luminance params.z
#define log_luminance_range params.w

shared uint shared_histogram[256];

void main()
{

    uint bin_count = histogram[gl_LocalInvocationIndex];
    shared_histogram[gl_LocalInvocationIndex] = bin_count * gl_LocalInvocationIndex;

    groupMemoryBarrier();
    barrier();

    histogram[gl_LocalInvocationIndex] = 0; // for next frame

    uint cutoff = 128;
    if(uint(gl_LocalInvocationIndex) < cutoff)
        atomicAdd(shared_histogram[gl_LocalInvocationIndex], shared_histogram[gl_LocalInvocationIndex + cutoff]);

    groupMemoryBarrier();
    barrier();

    cutoff = 64;
    if(uint(gl_LocalInvocationIndex) < cutoff)
        atomicAdd(shared_histogram[gl_LocalInvocationIndex], shared_histogram[gl_LocalInvocationIndex + cutoff]);

    groupMemoryBarrier();
    barrier();

    cutoff = 32;
    if(uint(gl_LocalInvocationIndex) < cutoff)
        atomicAdd(shared_histogram[gl_LocalInvocationIndex], shared_histogram[gl_LocalInvocationIndex + cutoff]);

    groupMemoryBarrier();
    barrier();

    cutoff = 16;
    if(uint(gl_LocalInvocationIndex) < cutoff)
        atomicAdd(shared_histogram[gl_LocalInvocationIndex], shared_histogram[gl_LocalInvocationIndex + cutoff]);

    groupMemoryBarrier();
    barrier();

    cutoff = 8;
    if(uint(gl_LocalInvocationIndex) < cutoff)
        atomicAdd(shared_histogram[gl_LocalInvocationIndex], shared_histogram[gl_LocalInvocationIndex + cutoff]);

    groupMemoryBarrier();
    barrier();

    cutoff = 4;
    if(uint(gl_LocalInvocationIndex) < cutoff)
        atomicAdd(shared_histogram[gl_LocalInvocationIndex], shared_histogram[gl_LocalInvocationIndex + cutoff]);

    groupMemoryBarrier();
    barrier();

    cutoff = 2;
    if(uint(gl_LocalInvocationIndex) < cutoff)
        atomicAdd(shared_histogram[gl_LocalInvocationIndex], shared_histogram[gl_LocalInvocationIndex + cutoff]);

    groupMemoryBarrier();
    barrier();

    cutoff = 1;
    if(uint(gl_LocalInvocationIndex) < cutoff)
        atomicAdd(shared_histogram[gl_LocalInvocationIndex], shared_histogram[gl_LocalInvocationIndex + cutoff]);

    groupMemoryBarrier();
    barrier();

    if(gl_GlobalInvocationID.x == 0)
    {
        // Do not count black pixels -> - float(bin_count)
        float weighted_log_average = (shared_histogram[0] / max(pixel_count - float(bin_count), 1.0)) - 1.0;

        float luminance_weighted_average = exp2(((weighted_log_average / 254.0) * log_luminance_range) + min_log_luminance);

        // smooth out with last frames luminance
        float last_frame = luminance;
        float smoothed = last_frame + (luminance_weighted_average - last_frame) * time_coefficient;
        luminance = smoothed;
    }
}
