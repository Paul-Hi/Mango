#version 430 core

// Vertex Input.
layout(location = 0) in vec3 vertex_data_position;
layout(location = 1) in vec3 vertex_data_normal;
layout(location = 2) in vec2 vertex_data_texcoord;
layout(location = 3) in vec4 vertex_data_tangent;

// Uniform Buffer Renderer.
layout(binding = 0, std140) uniform renderer_data
{
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    float camera_exposure;
};

// Uniform Buffer Model.
layout(binding = 1, std140) uniform model_data
{
    mat4 model_matrix;
    mat3 normal_matrix;
    bool has_normals;
    bool has_tangents;
};

// Shared Shader Data.
out shared_data
{
    vec3 position;
    vec2 texcoord;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
} vs_out;

vec4 get_world_position()
{
    return model_matrix * vec4(vertex_data_position, 1.0);
}

vec2 get_texture_coordinates()
{
    return vertex_data_texcoord;
}

void get_normal_tangent_bitangent(out vec3 normal, out vec3 tangent, out vec3 bitangent)
{
    if(has_normals)
        normal = normal_matrix * normalize(vertex_data_normal);
    if(has_tangents)
    {
        tangent = normal_matrix * normalize(vertex_data_tangent.xyz);

        if(has_normals)
        {
            bitangent = cross(normal, tangent);
            if(vertex_data_tangent.w == -1.0) // TODO Paul: Check this convention.
                bitangent *= -1.0;
        }
    }
}

// Passing relevant data to next shader.
void pass_shared_data()
{
    // Perspective Division
    vec4 world_position = get_world_position();
    vs_out.position = world_position.xyz / world_position.w;

    // Texture Coordinates
    vs_out.texcoord = get_texture_coordinates();

    // Normals, Tangents, Bitangents
    get_normal_tangent_bitangent(vs_out.normal, vs_out.tangent, vs_out.bitangent);

}

// Returns the gl_Position.
vec4 get_gl_position()
{
    vec4 world_position = get_world_position();
    return view_projection_matrix * world_position;
}

void main()
{
    pass_shared_data();
    gl_Position = get_gl_position();
}
