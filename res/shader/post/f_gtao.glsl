#include <../include/common_constants_and_functions.glsl>
#include <../include/camera.glsl>
#include <../include/gtao_data.glsl>
// https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf

out float ao;

in vec2 texcoord;

layout(binding = 0) uniform sampler2D sampler_hierarchical_depth; // texture "texture_hierarchical_depth"
layout(binding = 1) uniform sampler2D sampler_normal; // texture "texture_normal"

// See https://github.com/martymcmodding/qUINT/blob/master/Shaders/qUINT_mxao.fx and https://blog.selfshadow.com/publications/s2016-shading-course/activision/s2016_pbs_activision_occlusion.pdf in case we want to extend to multi bounce with color
float gtao_bw_multi_bounce(in float v)
{
    return max(v, ((v * 1.708 - 4.1534) * v + 3.4455) * v);
}

// TODO: Maybe check Bent Normals for directional occlusion and a little bit of GI
void main()
{
    float center_depth = textureLod(sampler_hierarchical_depth, texcoord, 1).x * 0.99999; // offset because self intersection

    vec3 view_normal = normalize((view_matrix * vec4(texture(sampler_normal, texcoord).xyz * 2.0 - 1.0, 0.0)).xyz);
    vec3 view_pos = view_space_from_depth(center_depth, texcoord, inverse_projection_matrix);
    vec3 view_dir = normalize(-view_pos);
    float radius = 0.25 * ao_radius / (direction_samples * 2 *(abs(view_pos.z) + 2.0));
    float falloff = -2.0 / (0.125 * ao_radius + 0.25 * ao_radius * ao_radius);

    const ivec2 fragcoord = ivec2(gl_FragCoord.xy);
    float spatial_direction_noise = (1.0 / 16.0) * ((((fragcoord.x + fragcoord.y) & 0x3) << 2) + (fragcoord.x & 0x3));
    float spatial_offset_noise = (1.0 / 4.0) * ((fragcoord.y - fragcoord.x) & 0x3);

    float visibility = 0;

    for (int slice = 0; slice < slices; ++slice)
    {
        float phi = PI * (float(slice) + spatial_direction_noise) / slices;

        vec2 omega = vec2(cos(phi), sin(phi));
        vec3 direction = vec3(omega, 0.0);
        omega *= radius;

        vec3 ortho_direction = direction - dot(direction, view_dir) * view_dir;
        vec3 view_axis = cross(direction, view_dir);
        vec3 projected_view_normal = view_normal - view_axis * dot(view_normal, view_axis);
        float pvnl = sqrt(dot(projected_view_normal, projected_view_normal));

        float sign_n = sign(dot(ortho_direction, projected_view_normal));
        float cos_n = saturate(dot(projected_view_normal, view_dir) / pvnl);
	    float n = sign_n * fast_acos(cos_n);

        // two sides -> two cos horizon -> two horizon angles
        float horizon_cos0 = -1.0;
        float horizon_cos1 = -1.0;

        for (int dir_sample = 0; dir_sample < direction_samples; ++dir_sample)
        {
            float s = float(dir_sample) / direction_samples;
            float scaling = fract(spatial_offset_noise + float(slice + dir_sample * direction_samples) * 10);
            vec2 offset = s * scaling * omega;
            vec2 s_texcoord0 = texcoord - offset;
            vec2 s_texcoord1 = texcoord + offset;

            // Mip calculation from https://github.com/martymcmodding/qUINT/blob/master/Shaders/qUINT_mxao.fx
            float mip = saturate(radius * dir_sample * 20.0) * 3.0 + 1.0; // add additional mip bias?
            float s_depth0 = textureLod(sampler_hierarchical_depth, s_texcoord0, mip).x;
            float s_depth1 = textureLod(sampler_hierarchical_depth, s_texcoord1, mip).x;
            vec3 s_pos0 = view_space_from_depth(s_depth0, s_texcoord0, inverse_projection_matrix);
            vec3 s_pos1 = view_space_from_depth(s_depth1, s_texcoord1, inverse_projection_matrix);

            vec3 s_hv0 = s_pos0 - view_pos;
            vec3 s_hv1 = s_pos1 - view_pos;
            float s_hd0 = sqrt(dot(s_hv0, s_hv0));
            float s_hd1 = sqrt(dot(s_hv1, s_hv1));
            s_hv0 /= s_hd0;
            s_hv1 /= s_hd1;

            float s_h_cos0 = dot(s_hv0, view_dir);
            float s_h_cos1 = dot(s_hv1, view_dir);

            // bounding the sampling area
            float weight0 = saturate(1.0 + falloff * s_hd0);
            float weight1 = saturate(1.0 + falloff * s_hd1);
            s_h_cos0 = mix(-1.0, s_h_cos0, weight0);
            s_h_cos1 = mix(-1.0, s_h_cos1, weight1);

            // thickness heuristic
            horizon_cos0 = (s_h_cos0 < horizon_cos0) ? mix(horizon_cos0, s_h_cos0, thin_occluder_compensation) : s_h_cos0;
            horizon_cos1 = (s_h_cos1 < horizon_cos1) ? mix(horizon_cos1, s_h_cos1, thin_occluder_compensation) : s_h_cos1;
        }

        float h0 = n + clamp(-fast_acos(horizon_cos0) - n, -HALF_PI, HALF_PI);
        float h1 = n + clamp(fast_acos(horizon_cos1) - n, -HALF_PI, HALF_PI);
        h0 *= 2.0;
        h1 *= 2.0;
        float sin_n = sin(n);

        visibility += pvnl * (cos_n + h0 * sin_n - cos(h0 - n)) * 0.25;
        visibility += pvnl * (cos_n + h1 * sin_n - cos(h1 - n)) * 0.25;
    }

    ao = visibility / slices;
    ao = saturate(multi_bounce ? gtao_bw_multi_bounce(ao) : ao);
    ao = pow(ao, power);
}
