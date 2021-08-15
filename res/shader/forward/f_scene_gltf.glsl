#include <../include/scene_geometry.glsl>

void main()
{
    gbuffer_color_target0 = vec4(get_base_color());
    gbuffer_color_target1 = vec4(get_normal() * 0.5 + 0.5, 1.0);
    gbuffer_color_target2 = vec4(get_emissive() * emissive_intensity, 1.0);
    gbuffer_color_target3 = vec4(get_occlusion_roughness_metallic(), 1.0);
}
