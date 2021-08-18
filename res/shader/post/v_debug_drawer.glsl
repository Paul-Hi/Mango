#include <../include/camera.glsl>

layout(location = 0) in vec3 vertex_data_position;
layout(location = 1) in vec3 vertex_data_color;

out vec3 shared_color;

void main()
{
    shared_color = vertex_data_color;
    gl_Position = view_projection_matrix * vec4(vertex_data_position, 1.0);
}
