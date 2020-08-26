#version 430 core

#define saturate(x) clamp(x, 0.0, 1.0)
#define group_size 256

layout(local_size_x = 256) in;

layout(std430, binding = 0) buffer luminance_data
{
    uint histogram[256];
    float luminance;
};

layout(location = 0) uniform vec4 params; // time coefficient (x), pixel_count (y), min_log_luminance (z), log_luminance_range (w)

shared uint shared_histogram[256];

void main()
{

    uint bin_count = histogram[gl_LocalInvocationIndex];
    shared_histogram[gl_LocalInvocationIndex] = bin_count * gl_LocalInvocationIndex;

    groupMemoryBarrier();
    barrier();

    histogram[gl_LocalInvocationIndex] = 0; // for next frame

    for(uint cutoff = (group_size >> 1); cutoff > 0; cutoff >>= 1)
    {
        if(uint(gl_LocalInvocationIndex) < cutoff)
            atomicAdd(shared_histogram[gl_LocalInvocationIndex], shared_histogram[gl_LocalInvocationIndex + cutoff]);

        groupMemoryBarrier();
        barrier();
    }

    if(gl_GlobalInvocationID.x == 0)
    {
        // Do not count black pixels -> - float(bin_count)
        float weighted_log_average = (shared_histogram[0] / max(params.y - float(bin_count), 1.0)) - 1.0;

        float luminance_weighted_average = exp2(((weighted_log_average / 254.0) * params.w) + params.z);

        // smooth out with last frames luminance
        float last_frame = luminance;
        float smoothed = last_frame + (luminance_weighted_average - last_frame) * params.x;
        luminance = smoothed;
    }
}
