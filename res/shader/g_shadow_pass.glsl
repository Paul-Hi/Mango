#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;

#define max_cascades 4

// Uniform Buffer Shadow.
layout(binding = 4, std140) uniform shadow_data
{
    mat4  view_projection_matrices[max_cascades];
    float split_depth[max_cascades + 1];
    vec4  far_planes;
    int   resolution;
    int   cascade_count;
    float shadow_cascade_interpolation_range;
    float maximum_penumbra;
};

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
    for(int layer = 0; layer < cascade_count; ++layer)
    {
        gl_Layer = layer;

        mat4 view_projection_matrix = view_projection_matrices[layer];
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
