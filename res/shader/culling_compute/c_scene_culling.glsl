
layout(local_size_x = 64) in;

#include <../include/common_constants_and_functions.glsl>

#define BIND_INDIRECT_COMMANDS_BUFFER
#define BIND_CULL_DATA_BUFFER
#define BIND_AABB_DATA_BUFFER

#include <../include/binding_data.glsl>

layout(binding = DEPTH_REDUCTION_TEXTURE_SAMPLER) uniform sampler2D depth_buffer_sampler_in; // texture "depth_buffer_texture_in"

bool intersect_frustum_aabb(vec4 corners[8]) {

    int outside[6] = { 0, 0, 0, 0, 0, 0 };

    for (int i = 0; i < 8; ++i)
    {
        if (corners[i].x >  corners[i].w) ++outside[0];
        if (corners[i].x < -corners[i].w) ++outside[1];
        if (corners[i].y >  corners[i].w) ++outside[2];
        if (corners[i].y < -corners[i].w) ++outside[3];
        if (corners[i].z >  corners[i].w) ++outside[4];
        if (corners[i].z < -corners[i].w) ++outside[5];
    }

    for (int i = 0; i < 6; ++i)
        if (outside[i] == 8) return false;

    return true;
}

bool is_occluded(vec4 corners[8]) {
    vec4 h_pos   = corners[0];
    h_pos.xyz    = clamp(h_pos.xyz, -h_pos.w, h_pos.w);
    vec3 clipmin = h_pos.xyz / h_pos.w;
    vec2 clipmax = clipmin.xz;
    for (int i = 1; i < 8; ++i)
    {
        vec4 h_pos      = corners[i];
        h_pos.xyz       = clamp(h_pos.xyz, -h_pos.w, h_pos.w);
        vec3 projected  = h_pos.xyz / h_pos.w;
        clipmin         = min(projected.xyz, clipmin);
        clipmax         = max(projected.xy, clipmax);
    }
    clipmin = clamp(clipmin * 0.5 + 0.5, 0.0, 1.0);
    clipmax = clamp(clipmax * 0.5 + 0.5, 0.0, 1.0);

    // xy uvs are 0-1
    // z in depth texture is 0-1

    vec4 box_uvs = vec4(clipmin.xy, clipmax.xy);
    vec4 box_uvs_viewport = box_uvs * data.xyxy;

    vec2 view_size = box_uvs_viewport.zw - box_uvs_viewport.xy;

    float lod = ceil(log2(max(view_size.x, view_size.y)));

    lod = clamp(lod, 0.0, data.z);

    float lower_lod = max(lod - 1, 0);
    float scale = exp2(-lower_lod);
    vec2 a = floor(box_uvs_viewport.xy * scale);
    vec2 b = ceil(box_uvs_viewport.zw * scale);
    vec2 dims = b - a;

    if (dims.x <= 2 && dims.y <= 2)
    {
        lod = lower_lod;
    }

    if (lod < 8) return true; // Even when not occluded this thing is too small to get noticed

    float d0 = textureLod(depth_buffer_sampler_in, box_uvs.xy, lod).r;
    float d1 = textureLod(depth_buffer_sampler_in, box_uvs.zy, lod).r;
    float d2 = textureLod(depth_buffer_sampler_in, box_uvs.xw, lod).r;
    float d3 = textureLod(depth_buffer_sampler_in, box_uvs.zw, lod).r;

    float max_depth = max(d0, max(d1, max(d2, d3)));

    if(clipmin.z <= max_depth)
        return false;

    return true;
}

void main()
{
    uint gID = gl_GlobalInvocationID.x;
    if (gID >= cull_draw_count)
        return;

    aabb bounds = cull_aabbs[gID];

    vec4 ndc_corners[8] = {
        cull_view_projection_matrix * vec4(bounds.center.x + bounds.extents.x, bounds.center.y + bounds.extents.y, bounds.center.z + bounds.extents.z, 1.0),
        cull_view_projection_matrix * vec4(bounds.center.x + bounds.extents.x, bounds.center.y + bounds.extents.y, bounds.center.z - bounds.extents.z, 1.0),
        cull_view_projection_matrix * vec4(bounds.center.x + bounds.extents.x, bounds.center.y - bounds.extents.y, bounds.center.z + bounds.extents.z, 1.0),
        cull_view_projection_matrix * vec4(bounds.center.x + bounds.extents.x, bounds.center.y - bounds.extents.y, bounds.center.z - bounds.extents.z, 1.0),
        cull_view_projection_matrix * vec4(bounds.center.x - bounds.extents.x, bounds.center.y + bounds.extents.y, bounds.center.z + bounds.extents.z, 1.0),
        cull_view_projection_matrix * vec4(bounds.center.x - bounds.extents.x, bounds.center.y + bounds.extents.y, bounds.center.z - bounds.extents.z, 1.0),
        cull_view_projection_matrix * vec4(bounds.center.x - bounds.extents.x, bounds.center.y - bounds.extents.y, bounds.center.z + bounds.extents.z, 1.0),
        cull_view_projection_matrix * vec4(bounds.center.x - bounds.extents.x, bounds.center.y - bounds.extents.y, bounds.center.z - bounds.extents.z, 1.0)
    };

    bool visible = true;

    // frustum culling
    visible = cull_frustum ? intersect_frustum_aabb(ndc_corners) : true;

    // occlusion culling
    visible = visible && (cull_occlusion ? !is_occluded(ndc_corners) : visible);

    if(visible)
    {
        atomicAdd(indirect_draws[cull_draws_offset + gID].instance_count, 1);
    }
}
