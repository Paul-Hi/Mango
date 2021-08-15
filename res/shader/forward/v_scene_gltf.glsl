#include <../include/scene_geometry.glsl>

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

void main()
{
    vec4 world_position = model_matrix * vec4(vertex_data_position, 1.0);

    // Perspective Division
    vs_out.position = world_position.xyz / world_position.w;

    // Texture Coordinates
    vs_out.texcoord = vertex_data_texcoord;

    // Normals, Tangents, Bitangents
    get_normal_tangent_bitangent(vs_out.normal, vs_out.tangent, vs_out.bitangent);

    gl_Position = view_projection_matrix * world_position;
}
