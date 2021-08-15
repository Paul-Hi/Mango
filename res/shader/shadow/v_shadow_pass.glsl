#include <../include/bindings.glsl>

layout(location = VERTEX_INPUT_POSITION) in vec3 vertex_data_position;
layout(location = VERTEX_INPUT_NORMAL) in vec3 vertex_data_normal;
layout(location = VERTEX_INPUT_TEXCOORD) in vec2 vertex_data_texcoord;
layout(location = VERTEX_INPUT_TANGENT) in vec4 vertex_data_tangent;

#include <../include/model.glsl>

out shared_data
{
    vec2 texcoord;
} vs_out;

vec4 get_world_position()
{
    return model_matrix * vec4(vertex_data_position, 1.0);
}

void main()
{
    vs_out.texcoord = vertex_data_texcoord;
    vec4 world_position = get_world_position();
    gl_Position = world_position;
}
