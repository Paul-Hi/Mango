
out vec3 frag_color;

in vec2 texcoord;

layout(binding = 0) uniform sampler2D sampler_input; // texture "texture_input"

void main()
{
    frag_color = textureLod(sampler_input, texcoord, 0).rgb;
}
