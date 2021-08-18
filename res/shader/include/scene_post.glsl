#ifndef MANGO_SCENE_POST_GLSL
#define MANGO_SCENE_POST_GLSL

#include <bindings.glsl>
#include <common_constants_and_functions.glsl>

#ifdef COMPOSING

out vec4 frag_color;

in vec2 texcoord;

layout(binding = COMPOSING_HDR_SAMPLER) uniform sampler2D sampler_hdr_input; // texture "texture_hdr_input"
layout(binding = COMPOSING_DEPTH_SAMPLER) uniform sampler2D sampler_geometry_depth_input; // texture "texture_geometry_depth_input"

#include <renderer.glsl>

#include <camera.glsl>

#endif // COMPOSING

#endif // MANGO_SCENE_POST_GLSL