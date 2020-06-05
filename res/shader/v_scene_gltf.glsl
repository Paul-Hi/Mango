#version 430 core

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texcoord;
layout(location = 3) in vec4 v_tangent;

layout(location = 0) uniform mat4 u_view_projection_matrix;

layout(binding = 0, std140) uniform scene_vertex_uniforms
{
    mat4 u_model_matrix;
    mat3 u_normal_matrix;
};

out shader_shared
{
    vec3 shared_normal;
    vec2 shared_texcoord;
    vec3 shared_tangent;
    vec3 shared_bitangent;
} vs_out;

void main()
{
    if(length(v_normal.xyz) > 0.0)
        vs_out.shared_normal = u_normal_matrix * normalize(v_normal);
    else
    {
        // TODO Paul: This is just for now and should be changed later. We want to calculate normals on import.
        vs_out.shared_normal = u_normal_matrix * vec3(0.0, 1.0, 0.0);
    }
    vs_out.shared_texcoord = v_texcoord;

    if(length(v_tangent.xyz) > 0.0)
    {
        vs_out.shared_tangent = u_normal_matrix * normalize(v_tangent.xyz);
        vs_out.shared_bitangent = cross(vs_out.shared_normal, vs_out.shared_tangent);
        if(v_tangent.w == -1.0) // TODO Paul: Check this convention.
            vs_out.shared_bitangent *= -1.0;
    }
    else
    {
        // TODO Paul: This calculation is just for now and should be changed later. We want to calculate tangents on import.
        vec3 absnormal = abs(vs_out.shared_normal);
        float max = max(absnormal.x, max(absnormal.y, absnormal.z));
        vec3 v = vs_out.shared_normal * step(max, absnormal);
        vs_out.shared_tangent = normalize(cross(vs_out.shared_normal, v));
        vs_out.shared_bitangent = cross(vs_out.shared_normal, vs_out.shared_tangent);
    }
    gl_Position = u_view_projection_matrix * u_model_matrix * vec4(v_position, 1.0);
}