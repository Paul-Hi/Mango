#version 430 core

out vec4 frag_color;

in vec3 shared_texcoord;

// layout (location = 1, binding = 0) uniform samplerCube skybox;
//
// void main()
// {
//     frag_color = texture(skybox, shared_texcoord);
// }

layout (location = 1, binding = 0) uniform sampler2D skybox;

const vec2 inv_atan = vec2(0.1591, 0.3183);
vec2 sample_spherical_map(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= inv_atan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec3 pos = normalize(shared_texcoord);
    pos.y *= -1.0;
    vec2 uv = sample_spherical_map(pos);
    vec3 color = texture(skybox, uv).rgb;

    color = pow(color, vec3(1.0 / 2.2));
    frag_color = vec4(color, 1.0);
}
