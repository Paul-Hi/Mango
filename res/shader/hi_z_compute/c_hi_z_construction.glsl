#include <../include/bindings.glsl>
#include <../include/hi_z.glsl>

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = HI_Z_DEPTH_SAMPLER) uniform sampler2D sampler_depth_input; // texture "texure_depth_input"
layout(binding = HI_Z_IMAGE_COMPUTE, rg32f) uniform writeonly image2D image_hi_z_output;

#define validate_fetch(coords, size) clamp(coords, ivec2(0,0), ivec2(size) - 1)

void main()
{
    ivec2 coords = ivec2(gl_GlobalInvocationID.xy);

    if (coords.x < params.x && coords.y < params.y)
    {

        if (pass == 0)
        {
            vec2 uv = (coords + 0.5) * params.zw;
            float depth = texture(sampler_depth_input, uv).x;

            imageStore(image_hi_z_output, coords, vec4(depth, depth, 0.0, 0.0));
        }
        else
        {
            ivec2 offsets = ivec2(0, 1);
            ivec2 old_coords = coords * 2;
            vec2[4] depths = {
                                texelFetch(sampler_depth_input, validate_fetch(old_coords + offsets.xx, params.zw), pass - 1).rg,
                                texelFetch(sampler_depth_input, validate_fetch(old_coords + offsets.xy, params.zw), pass - 1).rg,
                                texelFetch(sampler_depth_input, validate_fetch(old_coords + offsets.yx, params.zw), pass - 1).rg,
                                texelFetch(sampler_depth_input, validate_fetch(old_coords + offsets.yy, params.zw), pass - 1).rg
            };

            vec4 depths_max = vec4(
                                    depths[0].r,
                                    depths[1].r,
                                    depths[2].r,
                                    depths[3].r
            );

            vec4 depths_min = vec4(
                                    depths[0].g,
                                    depths[1].g,
                                    depths[2].g,
                                    depths[3].g
            );

            float depth_max = max(max(depths_max.x, depths_max.y), max(depths_max.z, depths_max.w));
            float depth_min = min(min(depths_min.x, depths_min.y), min(depths_min.z, depths_min.w));

            // additional cols/rows when odd
            bool extra_col = ((int(params.z) & 1) != 0);
            bool extra_row = ((int(params.w) & 1) != 0);
            if (extra_col) {
                vec2[2] extra_col_depths = {
                                texelFetch(sampler_depth_input, validate_fetch(old_coords + ivec2(2, 0), params.zw), pass - 1).rg,
                                texelFetch(sampler_depth_input, validate_fetch(old_coords + ivec2(2, 1), params.zw), pass - 1).rg
                };

                vec2 extra_col_max = vec2(
                                    extra_col_depths[0].r,
                                    extra_col_depths[1].r
                );

                vec2 extra_col_min = vec2(
                                    extra_col_depths[0].g,
                                    extra_col_depths[1].g
                );

                if (extra_row) {
                    vec2 corner_depths = texelFetch(sampler_depth_input, validate_fetch(old_coords + ivec2(2, 2), params.zw), pass - 1).rg;

                    depth_max = max(depth_max, corner_depths.r);
                    depth_min = min(depth_min, corner_depths.g);
                }

                depth_max = max(depth_max, max(extra_col_max.x, extra_col_max.y));
                depth_min = min(depth_min, min(extra_col_min.x, extra_col_min.y));
            }

            if (extra_row) {
                vec2[2] extra_row_depths = {
                                texelFetch(sampler_depth_input, validate_fetch(old_coords + ivec2(0, 2), params.zw), pass - 1).rg,
                                texelFetch(sampler_depth_input, validate_fetch(old_coords + ivec2(1, 2), params.zw), pass - 1).rg
                };

                vec2 extra_row_max = vec2(
                                    extra_row_depths[0].r,
                                    extra_row_depths[1].r
                );

                vec2 extra_row_min = vec2(
                                    extra_row_depths[0].g,
                                    extra_row_depths[1].g
                );

                depth_max = max(depth_max, max(extra_row_max.x, extra_row_max.y));
                depth_min = min(depth_min, min(extra_row_min.x, extra_row_min.y));
            }

            imageStore(image_hi_z_output, coords, vec4(depth_max, depth_min, 0.0, 0.0));
        }

    }
}
