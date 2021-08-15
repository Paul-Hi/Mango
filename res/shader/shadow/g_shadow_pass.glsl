layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;

#define max_cascades 4

#include <../include/shadow.glsl>

in shared_data
{
    vec2 texcoord;
} gs_in[];

out shared_data
{
    vec2 texcoord;
} gs_out;

void main()
{
    for(int layer = 0; layer < shadow_cascade_count; ++layer)
    {
        gl_Layer = layer;

        mat4 view_projection_matrix = shadow_view_projection_matrices[layer];
        for(int i = 0; i < gl_in.length(); ++i)
        {
            vec4 pos = view_projection_matrix * gl_in[i].gl_Position;
            gl_Position = pos;
            gs_out.texcoord = gs_in[i].texcoord;
            EmitVertex();
        }
        EndPrimitive();
    }
}
