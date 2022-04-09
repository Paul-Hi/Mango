
layout(local_size_x = 64) in;

#define BIND_INDIRECT_COMMANDS_BUFFER
#define BIND_CULL_DATA_BUFFER
#define BIND_AABB_DATA_BUFFER

#include <../include/binding_data.glsl>

bool intersect_frustum_aabb(vec4 cull_camera_frustum_planes[6], aabb bounds) {
    vec3 corners[8] = {
        vec3(bounds.center.x + bounds.extents.x, bounds.center.y + bounds.extents.y, bounds.center.z + bounds.extents.z),
        vec3(bounds.center.x + bounds.extents.x, bounds.center.y + bounds.extents.y, bounds.center.z - bounds.extents.z),
        vec3(bounds.center.x + bounds.extents.x, bounds.center.y - bounds.extents.y, bounds.center.z + bounds.extents.z),
        vec3(bounds.center.x + bounds.extents.x, bounds.center.y - bounds.extents.y, bounds.center.z - bounds.extents.z),
        vec3(bounds.center.x - bounds.extents.x, bounds.center.y + bounds.extents.y, bounds.center.z + bounds.extents.z),
        vec3(bounds.center.x - bounds.extents.x, bounds.center.y + bounds.extents.y, bounds.center.z - bounds.extents.z),
        vec3(bounds.center.x - bounds.extents.x, bounds.center.y - bounds.extents.y, bounds.center.z + bounds.extents.z),
        vec3(bounds.center.x - bounds.extents.x, bounds.center.y - bounds.extents.y, bounds.center.z - bounds.extents.z)
    };

    for (int i = 0; i < 6; ++i)
    {
        /*
        bool inside = false;
        for (int j = 0; j < 8; ++j)
        {
            if (dot(cull_camera_frustum_planes[i], vec4(corners[j].xyz, 1.0)) >= 0.0)
            {
                inside = true;
                break;
            }
        }

        if (!inside)
            return false;
        */

        int outside = 0;
        outside += (dot(cull_camera_frustum_planes[i], vec4(corners[0].xyz, 1.0)) < 0.0) ? 1 : 0;
        outside += (dot(cull_camera_frustum_planes[i], vec4(corners[1].xyz, 1.0)) < 0.0) ? 1 : 0;
        outside += (dot(cull_camera_frustum_planes[i], vec4(corners[2].xyz, 1.0)) < 0.0) ? 1 : 0;
        outside += (dot(cull_camera_frustum_planes[i], vec4(corners[3].xyz, 1.0)) < 0.0) ? 1 : 0;
        outside += (dot(cull_camera_frustum_planes[i], vec4(corners[4].xyz, 1.0)) < 0.0) ? 1 : 0;
        outside += (dot(cull_camera_frustum_planes[i], vec4(corners[5].xyz, 1.0)) < 0.0) ? 1 : 0;
        outside += (dot(cull_camera_frustum_planes[i], vec4(corners[6].xyz, 1.0)) < 0.0) ? 1 : 0;
        outside += (dot(cull_camera_frustum_planes[i], vec4(corners[7].xyz, 1.0)) < 0.0) ? 1 : 0;
        if(outside == 8) return false;
    }

    return true;
}


void main()
{
    uint gID = gl_GlobalInvocationID.x;
    if (gID >= cull_draw_count)
        return;

    bool visible = intersect_frustum_aabb(cull_camera_frustum_planes, cull_aabbs[gID]);

    if(visible)
    {
        atomicAdd(indirect_draws[cull_draws_offset + gID].instance_count, 1);
    }
}
