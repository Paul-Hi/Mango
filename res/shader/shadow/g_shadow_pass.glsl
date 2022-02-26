layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

#define BIND_SHADOW_DATA_BUFFER

#include <../include/binding_data.glsl>

in shared_data
{
    vec2 texcoord;
    flat ivec2 draw_id; // (model_data index, material_data index)
} gs_in[];

out shared_data
{
    vec2 texcoord;
    flat ivec2 draw_id; // (model_data index, material_data index)
} gs_out;

void main()
{
    gl_Layer = cascade;
    mat4 view_projection_matrix = shadow_view_projection_matrices[cascade];
    for(int i = 0; i < gl_in.length(); ++i)
    {
        vec4 pos = view_projection_matrix * gl_in[i].gl_Position;
        gl_Position = pos;
        gs_out.texcoord = gs_in[i].texcoord;
        gs_out.draw_id = gs_in[i].draw_id;
        EmitVertex();
    }
    EndPrimitive();
}
