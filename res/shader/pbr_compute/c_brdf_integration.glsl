
#include <../include/common_constants_and_functions.glsl>
#include <../include/pbr_functions.glsl>

const uint sample_count = 512;
float inverse_sample_count = 1.0 / float(sample_count);

layout(local_size_x = 32, local_size_y = 32) in;

layout(binding = 0, rgba16f) uniform writeonly image2D integration_lut_out;

layout(binding = 3) uniform ibl_generation_data
{
    vec2 out_size;
    vec2 data;
};

float inv_tex_size = 1.0 / out_size.x;

void main()
{
    ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
    if (coords.x >= out_size.x || coords.y >= out_size.y)
        return;
    float n_dot_v = float(coords.x) * inv_tex_size;
    float perceptual_roughness = float(coords.y) * inv_tex_size;
    float alpha = perceptual_roughness * perceptual_roughness;

    n_dot_v = clamp(n_dot_v, 1e-5, 1.0 - 1e-5);

    vec3 view = vec3(sqrt(1.0 - n_dot_v * n_dot_v), 0.0, n_dot_v);
    vec3 normal = vec3(0.0, 0.0, 1.0);

    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent_x = normalize(cross(up, normal));
    vec3 tangent_y = normalize(cross(normal, tangent_x));

    vec3 integration_lu = vec3(0.0);

    for(uint s = 0; s < sample_count; ++s)
    {
        vec2 eta = sample_hammersley(s, inverse_sample_count);
        vec3 halfway;
        importance_sample_ggx_direction(eta, normal, tangent_x, tangent_y, alpha, halfway);
        vec3 to_light = normalize(2.0 * dot(view, halfway) * halfway - view);
        float n_dot_l = to_light.z;
        float G = 4.0 * V_SmithGGXCorrelated(n_dot_v, n_dot_l, alpha);

        // specular preintegration with multiscattering
        if(n_dot_l > 0.0 && G > 0.0)
        {
            float v_dot_h = saturate(dot(view, halfway));
            float n_dot_h = saturate(halfway.z);
            float G_vis = G * n_dot_l * (v_dot_h / n_dot_h );
            float Fc = pow5(1.0 - v_dot_h);
            // https://google.github.io/filament/Filament.html#listing_energycompensationimpl
            // Assuming f90 = 1
            integration_lu.x += Fc * G_vis;
            integration_lu.y += G_vis;
        }
    }

    integration_lu *= inverse_sample_count;

    imageStore(integration_lut_out, coords, vec4(integration_lu, 1.0));
}
