#ifndef MANGO_COMPOSING_GLSL
#define MANGO_COMPOSING_GLSL

#include <bindings.glsl>
#include <common_constants_and_functions.glsl>

#ifdef COMPOSING

out vec4 frag_color;

in vec2 texcoord;

layout(binding = COMPOSING_HDR_SAMPLER) uniform sampler2D sampler_hdr_input; // texture "texture_hdr_input"
layout(binding = COMPOSING_DEPTH_SAMPLER) uniform sampler2D sampler_geometry_depth_input; // texture "texture_geometry_depth_input"

#include <renderer.glsl>
#include <camera.glsl>

layout(binding = COMPOSING_DATA_BUFFER_BINDING_POINT, std140) uniform composing_data
{
    vec3 exposure_bias;
    vec3 tint;
    vec3 contrast;
    vec3 saturation;
    vec3 lift;
    vec3 gamma;
    vec3 gain;
};



#endif // COMPOSING

#endif // MANGO_COMPOSING_GLSL