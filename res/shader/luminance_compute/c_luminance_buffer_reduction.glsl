#include <../include/common_constants_and_functions.glsl>
#include <../include/bindings.glsl>

layout(local_size_x = 16, local_size_y = 16) in;

layout(std430, binding = LUMINANCE_DATA_BUFFER_BINDING_POINT) buffer luminance_data
{
    uint histogram[256];
    vec4 params; // min_log_luminance (x), inverse_log_luminance_range (y), time coefficient (z), pixel_count (w)
    float luminance;
};

#define min_log_luminance params.x
#define log_luminance_range (1.0 / params.y)
#define time_coefficient params.z
#define pixel_count params.w

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
        float weighted_log_average = (float(shared_histogram[0]) / max(pixel_count - float(bin_count), 1.0)) - 1.0;

        float luminance_weighted_average = exp2(((weighted_log_average / 254.0) * log_luminance_range) + min_log_luminance);

        // smooth out with last frames luminance
        float last_frame = luminance;
        float smoothed = last_frame + (luminance_weighted_average - last_frame) * time_coefficient;
        luminance = smoothed;
    }
}
