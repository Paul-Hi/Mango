layout (location = 0) in vec3 vertex_data_position;

// Uniform Buffer Renderer.
layout(binding = 0, std140) uniform renderer_data
{
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    float camera_exposure;
    bool shadow_step_enabled;
};

// Uniform Buffer Cubemap.
layout(binding = 5, std140) uniform cubemap_data
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
