#include <../include/scene_geometry.glsl>

void get_normal_tangent_bitangent(in mat3 normal_transform, out vec3 normal, out vec3 tangent, out vec3 bitangent)
{
    if(has_normals)
        normal = normal_transform * normalize(vertex_data_normal);
    if(has_tangents)
    {
        tangent = normal_transform * normalize(vertex_data_tangent.xyz);

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
    mat4 vertex_transform = model_matrix;
    mat3 normal_transform = normal_matrix;
    if(has_joints && has_weights)
    {
        mat4 skin_matrix = vertex_data_weight.x * joint_matrices[int(vertex_data_joint.x)] +
                           vertex_data_weight.y * joint_matrices[int(vertex_data_joint.y)] +
                           vertex_data_weight.z * joint_matrices[int(vertex_data_joint.z)] +
                           vertex_data_weight.w * joint_matrices[int(vertex_data_joint.w)];

        vertex_transform = skin_matrix;
        normal_transform = mat3(transpose(inverse(skin_matrix))); // TODO Paul: Transpose and inverse in shader sufficient?
    }

    vec4 world_position = vertex_transform * vec4(vertex_data_position, 1.0);


    // Perspective Division
    vs_out.position = world_position.xyz / world_position.w;

    // Texture Coordinates
    vs_out.texcoord = vertex_data_texcoord;

    // Normals, Tangents, Bitangents
    get_normal_tangent_bitangent(normal_transform, vs_out.normal, vs_out.tangent, vs_out.bitangent);

    gl_Position = view_projection_matrix * world_position;
}
