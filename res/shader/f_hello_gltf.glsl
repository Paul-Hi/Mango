#version 430 core

out vec4 frag_color;

in vec3 shared_normal;

void main()
{
    vec3 light_dir = normalize(vec3(0.5f));
    vec3 col = vec3(1.0f, 0.9f, 0.9f) * pow(dot(normalize(shared_normal), light_dir), 3.0f);
    frag_color = vec4(col, 1.0f);
}
