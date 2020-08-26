#version 430 core

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texcoord;
layout(location = 3) in vec4 v_tangent;

layout(location = 0) uniform mat4 view_projection_matrix;

layout(binding = 0, std140) uniform scene_vertex_uniforms
{
    mat4 model_matrix;
    mat3 normal_matrix;
    bool has_normals;
    bool has_tangents;
};

out shader_shared
{
    vec3 shared_vertex_position;
    vec2 shared_texcoord;
    vec3 shared_normal;
    vec3 shared_tangent;
    vec3 shared_bitangent;
    flat bool calculate_normals;
    flat bool calculate_tangents;
} vs_out;

void main()
{
    vec4 v_pos = model_matrix * vec4(v_position, 1.0);
    vs_out.shared_vertex_position = v_pos.xyz / v_pos.w;

    vs_out.shared_texcoord = v_texcoord;


    vs_out.calculate_normals  = false;
    vs_out.calculate_tangents = false;

    if(has_normals)
        vs_out.shared_normal = normal_matrix * normalize(v_normal);

    if(has_tangents)
    {
        vs_out.shared_tangent = normal_matrix * normalize(v_tangent.xyz);
        if(has_normals)
        {
            vs_out.shared_bitangent = cross(vs_out.shared_normal, vs_out.shared_tangent);
            if(v_tangent.w == -1.0) // TODO Paul: Check this convention.
                vs_out.shared_bitangent *= -1.0;
        }
    }

    gl_Position = view_projection_matrix * v_pos;
}
