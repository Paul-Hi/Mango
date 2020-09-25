#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;

#define max_cascades 4

layout(location = 0) uniform mat4 view_projection_matrices[max_cascades];
layout(location = 4) uniform int cascade_count;

void main()
{
    for(int layer = 0; layer < cascade_count; ++layer)
    {
        gl_Layer = layer;

        mat4 view_projection_matrix = view_projection_matrices[layer];
        for(int i = 0; i < gl_in.length(); ++i)
        {
            vec4 pos = view_projection_matrix * gl_in[i].gl_Position;
            gl_Position = pos;
            EmitVertex();
        }
        EndPrimitive();
    }
}