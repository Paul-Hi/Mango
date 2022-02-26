#include <../include/bindings.glsl>

layout(location = VERTEX_INPUT_POSITION) in vec3 vertex_data_position;
// layout(location = VERTEX_INPUT_NORMAL) in vec3 vertex_data_normal;
layout(location = VERTEX_INPUT_TEXCOORD) in vec2 vertex_data_texcoord;
// layout(location = VERTEX_INPUT_TANGENT) in vec4 vertex_data_tangent;
layout(location = VERTEX_INPUT_DRAW_ID) in ivec2 vertex_data_draw_id;

#define BIND_MODEL_DATA_BUFFER

#include <../include/binding_data.glsl>

out shared_data
{
    vec2 texcoord;
    flat ivec2 draw_id; // (model_data index, material_data index)
} vs_out;

vec4 get_world_position()
{
    int model_id = vertex_data_draw_id.x;
    per_model_data data = model_data_array[model_id];
    return data.model_matrix * vec4(vertex_data_position, 1.0);
}

void main()
{
    vs_out.texcoord = vertex_data_texcoord;
    vs_out.draw_id = vertex_data_draw_id;
    vec4 world_position = get_world_position();
    gl_Position = world_position;
}
