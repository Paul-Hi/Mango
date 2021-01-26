#include <../include/common_state.glsl>

void main()
{
    vec4 world_position = get_world_space_position();
    pass_shared_data(world_position);
    gl_Position = get_camera_view_projection_matrix() * world_position;
}
