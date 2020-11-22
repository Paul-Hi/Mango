#version 430 core

#define F16_MAX 65500.0
#define F16_MIN 0.000655

#define saturate(x) clamp(x, 0.0, 1.0)

const float PI = 3.1415926535897932384626433832795;

layout(local_size_x = 32, local_size_y = 32) in;

layout(binding = 0, rgba16f) uniform writeonly imageCube cubemap_out;

layout(location = 0) uniform vec2 out_size;

// Uniform Buffer Atmosphere Compute.
layout(binding = 6, std140) uniform atmosphere_ub_data
{
    vec4 sun_dir; // vec3 -> vec4
    vec4 rayleigh_scattering_coefficients; // vec3 -> vec4
    vec4 ray_origin; // vec3 -> vec4
    vec2 density_multiplier;
    float sun_intensity;
    float mie_scattering_coefficient;
    float ground_radius;
    float atmosphere_radius;
    float mie_preferred_scattering_dir;
    int scatter_points;
    int scatter_points_second_ray;
};


vec3 cube_to_world(in ivec3 cube_coord, in vec2 cubemap_size);
vec3 atmospheric_scattering(in vec3 ray_dir);

vec2 intersect_ray_sphere(vec3 origin, vec3 dir, float sphere_radius) {
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, origin);
    float c = dot(origin, origin) - (sphere_radius * sphere_radius);
    float d = (b * b) - 4.0 * a * c;

    if (d < 0.0)
        return vec2(1e5, -1e5);
    return vec2((-b - sqrt(d)) / (2.0 * a), (-b + sqrt(d)) / (2.0 * a));
}

void main()
{
    vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0);

    ivec3 coords = ivec3(gl_GlobalInvocationID);
    vec3 pos = cube_to_world(coords, out_size);

    pixel.rgb = atmospheric_scattering(normalize(pos));
    pixel.rgb = clamp(pixel.rgb, vec3(F16_MIN), vec3(F16_MAX));

    imageStore(cubemap_out, coords, pixel);
}

vec3 cube_to_world(in ivec3 cube_coord, in vec2 cubemap_size)
{
    vec2 tex_coord = vec2(cube_coord.xy + 0.5) / cubemap_size;
    tex_coord = tex_coord  * 2.0 - 1.0;
    switch(cube_coord.z)
    {
        case 0: return vec3(1.0, -tex_coord.yx);
        case 1: return vec3(-1.0, -tex_coord.y, tex_coord.x);
        case 2: return vec3(tex_coord.x, 1.0, tex_coord.y);
        case 3: return vec3(tex_coord.x, -1.0, -tex_coord.y);
        case 4: return vec3(tex_coord.x, -tex_coord.y, 1.0);
        case 5: return vec3(-tex_coord.xy, -1.0);
    }
    return vec3(0.0);
}

vec3 atmospheric_scattering(in vec3 ray_dir)
{
    vec2 intersection_data = intersect_ray_sphere(ray_origin.xyz, ray_dir, atmosphere_radius);
    vec2 ground_intersect = intersect_ray_sphere(ray_origin.xyz, ray_dir, ground_radius);

    // add a sun disc ... I can not cast good shadows with that ....
    // vec3 sun = vec3(pow(smoothstep(0.97, 1.0, dot(ray_dir, sun_dir.xyz)), 16.0));
    // sun *= saturate(pow(smoothstep(0.0, ground_radius, abs(ground_intersect.x) - ground_intersect.y), 2.0));

    if (intersection_data.x > intersection_data.y)
    {
        return vec3(0.0);
    }
    else
    {
        intersection_data.y = min(intersection_data.y, ground_intersect.x);
        float step_size = (intersection_data.y - intersection_data.x) / float(scatter_points);

        vec3 rayleigh_total = vec3(0.0);
        vec3 mie_total = vec3(0.0);
        vec2 accumulated_optical_depths = vec2(0.0);

        float a = dot(ray_dir, sun_dir.xyz);
        float a_sqr = a * a;
        float pd_srq = mie_preferred_scattering_dir * mie_preferred_scattering_dir;
        float rayleigh_phase = 3.0 / (16.0 * PI) * (1.0 + a_sqr);
        float mie_phase = 3.0 / (8.0 * PI) * ((1.0 - pd_srq) * (a_sqr + 1.0)) / (pow(1.0 + pd_srq - 2.0 * a * mie_preferred_scattering_dir, 1.5) * (2.0 + pd_srq));

        vec3 scatter_point = ray_origin.xyz + ray_dir * (step_size * 0.5);
        for(int i = 0; i < scatter_points; ++i)
        {
            float scatter_height = length(scatter_point) - ground_radius;

            vec2 optical_depths = exp(-scatter_height / density_multiplier) * step_size;
            accumulated_optical_depths += optical_depths;

            float second_step_size = intersect_ray_sphere(scatter_point, sun_dir.xyz, atmosphere_radius).y / float(scatter_points_second_ray);
            vec2 second_ray_accumulated_optical_depths = vec2(0.0);

            vec3 second_scattering_point = scatter_point + sun_dir.xyz * (second_step_size * 0.5);
            for(int j = 0; j < scatter_points_second_ray; ++j)
            {
                float second_height = length(second_scattering_point) - ground_radius;

                second_ray_accumulated_optical_depths += exp(-second_height / density_multiplier) * second_step_size;

                second_scattering_point += sun_dir.xyz * second_step_size;
            }

            vec3 attenuation = exp(-(mie_scattering_coefficient * (accumulated_optical_depths.y + second_ray_accumulated_optical_depths.y)
                    + rayleigh_scattering_coefficients.xyz * (accumulated_optical_depths.x + second_ray_accumulated_optical_depths.x)));

            rayleigh_total += optical_depths.x * attenuation;
            mie_total += optical_depths.y * attenuation;

            scatter_point += ray_dir * step_size;
        }

        return sun_intensity * (rayleigh_phase * rayleigh_scattering_coefficients.xyz * rayleigh_total
            + mie_phase * mie_scattering_coefficient * mie_total);// + sun * mie_total * 0.007);
    }
}
