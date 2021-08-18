#include <../include/camera.glsl>

layout(location = 0) in vec3 vertex_data_position;

// Uniform Buffer Cubemap.
layout(binding = 3, std140) uniform cubemap_data
{
    mat4 model_matrix;
    float render_level;
};

out vec3 shared_texcoord;

void main()
{
    shared_texcoord = vertex_data_position;
    mat4 view_no_translation = mat4(mat3(view_matrix));
    vec4 pos = projection_matrix * view_no_translation * model_matrix * vec4(vertex_data_position, 1.0);
    gl_Position = pos.xyww;
}
