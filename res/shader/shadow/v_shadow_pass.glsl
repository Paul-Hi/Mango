#include <../include/bindings.glsl>

layout(location = VERTEX_INPUT_POSITION) in vec3 vertex_data_position;
layout(location = VERTEX_INPUT_NORMAL) in vec3 vertex_data_normal;
layout(location = VERTEX_INPUT_TANGENT) in vec4 vertex_data_tangent;
layout(location = VERTEX_INPUT_TEXCOORD0) in vec2 vertex_data_texcoord;
layout(location = VERTEX_INPUT_JOINT) in vec4 vertex_data_joint;
layout(location = VERTEX_INPUT_WEIGHT) in vec4 vertex_data_weight;

#include <../include/model.glsl>

#include <../include/animation.glsl>

out shared_data
{
    vec2 texcoord;
} vs_out;

vec4 get_world_position()
{
    mat4 vertex_transform = model_matrix;
    if(has_joints && has_weights)
    {
        mat4 skin_matrix = vertex_data_weight.x * joint_matrices[int(vertex_data_joint.x)] +
                           vertex_data_weight.y * joint_matrices[int(vertex_data_joint.y)] +
                           vertex_data_weight.z * joint_matrices[int(vertex_data_joint.z)] +
                           vertex_data_weight.w * joint_matrices[int(vertex_data_joint.w)];

        // gltf implementation says: Ignore transformation of the mesh node, only apply transformation of skeleton node.
        vertex_transform = skin_matrix;
    }
    return vertex_transform * vec4(vertex_data_position, 1.0);
}

void main()
{
    vs_out.texcoord = vertex_data_texcoord;
    vec4 world_position = get_world_position();
    gl_Position = world_position;
}
