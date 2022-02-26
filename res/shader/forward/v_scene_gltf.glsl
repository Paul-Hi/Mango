#include <../include/scene_geometry.glsl>

void main()
{
    vec4 world_position = get_model_matrix() * vec4(vertex_data_position, 1.0);

    // Perspective Division
    vs_out.position = world_position.xyz / world_position.w;

    // Texture Coordinates
    vs_out.texcoord = vertex_data_texcoord;

    // DrawID
    vs_out.draw_id = vertex_data_draw_id;

    // Normals, Tangents, Bitangents
    get_normal_tangent_bitangent(vs_out.normal, vs_out.tangent, vs_out.bitangent);

    gl_Position = view_projection_matrix * world_position;
}
