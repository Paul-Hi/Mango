#ifndef MANGO_BINDINGS_GLSL
#define MANGO_BINDINGS_GLSL

#define RENDERER_DATA_BUFFER_BINDING_POINT 0
#define CAMERA_DATA_BUFFER_BINDING_POINT 1
#define MODEL_DATA_BUFFER_BINDING_POINT 2
#define MATERIAL_DATA_BUFFER_BINDING_POINT 3
#define LIGHT_DATA_BUFFER_BINDING_POINT 4
#define SHADOW_DATA_BUFFER_BINDING_POINT 5
#define LUMINANCE_DATA_BUFFER_BINDING_POINT 6
#define IBL_GEN_DATA_BUFFER_BINDING_POINT 3
#define CUBEMAP_DATA_BUFFER_BINDING_POINT 3
#define FXAA_DATA_BUFFER_BINDING_POINT 1
#define HI_Z_DATA_BUFFER_BINDING_POINT 6

#define VERTEX_INPUT_POSITION 0
#define VERTEX_INPUT_NORMAL 1
#define VERTEX_INPUT_TEXCOORD 2
#define VERTEX_INPUT_TANGENT 3

#define GBUFFER_OUTPUT_TARGET0 0
#define GBUFFER_OUTPUT_TARGET1 1
#define GBUFFER_OUTPUT_TARGET2 2
#define GBUFFER_OUTPUT_TARGET3 3

#define GBUFFER_TEXTURE_SAMPLER_TARGET0 0
#define GBUFFER_TEXTURE_SAMPLER_TARGET1 1
#define GBUFFER_TEXTURE_SAMPLER_TARGET2 2
#define GBUFFER_TEXTURE_SAMPLER_TARGET3 3
#define GBUFFER_TEXTURE_SAMPLER_DEPTH 4

#define GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR 0
#define GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC 1
#define GEOMETRY_TEXTURE_SAMPLER_OCCLUSION 2
#define GEOMETRY_TEXTURE_SAMPLER_NORMAL 3
#define GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR 4

#define IBL_SAMPLER_IRRADIANCE_MAP 5
#define IBL_SAMPLER_RADIANCE_MAP 6
#define IBL_SAMPLER_LOOKUP 7
#define SAMPLER_SHADOW_SHADOW_MAP 8
#define SAMPLER_SHADOW_MAP 9

#define COMPOSING_HDR_SAMPLER 0
#define COMPOSING_DEPTH_SAMPLER 1

#define HDR_IMAGE_LUMINANCE_COMPUTE 0

#define HI_Z_DEPTH_SAMPLER 0
#define HI_Z_IMAGE_COMPUTE 1

#endif // MANGO_BINDINGS_GLSL