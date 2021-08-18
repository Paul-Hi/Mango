out vec4 frag_color;

in vec3 shared_color;


void main()
{
    frag_color = vec4(shared_color, 1.0);
}