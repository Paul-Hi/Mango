layout(location = 0) in vec3 vertex_data_position;
layout(location = 1) in vec3 vertex_data_normal;
layout(location = 2) in vec2 vertex_data_texcoord;
layout(location = 3) in vec4 vertex_data_tangent;

// Uniform Buffer Model.
layout(binding = 2, std140) uniform model_data
{
    mat4 model_matrix;
    mat3 normal_matrix;
    bool has_normals;
    bool has_tangents;
};

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
