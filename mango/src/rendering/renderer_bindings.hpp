//! \file      renderer_bindings.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_RENDERER_BINDINGS_HPP
#define MANGO_RENDERER_BINDINGS_HPP

namespace mango
{
    //! \brief The binding point for the \a renderer_data buffer.
#define RENDERER_DATA_BUFFER_BINDING_POINT 0
    //! \brief The binding point for the \a camera_data buffer.
#define CAMERA_DATA_BUFFER_BINDING_POINT 1
    //! \brief The binding point for the \a model_data buffer.
#define MODEL_DATA_BUFFER_BINDING_POINT 2
    //! \brief The binding point for the \a material_data buffer.
#define MATERIAL_DATA_BUFFER_BINDING_POINT 3
    //! \brief The binding point for the \a light_data buffer.
#define LIGHT_DATA_BUFFER_BINDING_POINT 4
    //! \brief The binding point for the \a shadow_data buffer.
#define SHADOW_DATA_BUFFER_BINDING_POINT 5
    //! \brief The binding point for the \a luminance_data buffer.
#define LUMINANCE_DATA_BUFFER_BINDING_POINT 6
    //! \brief The binding point for the \a ibl_generation_data buffer.
#define IBL_GEN_DATA_BUFFER_BINDING_POINT 3
    //! \brief The binding point for the \a cubemap_data buffer.
#define CUBEMAP_DATA_BUFFER_BINDING_POINT 3
    //! \brief The binding point for the \a fxaa_data buffer.
#define FXAA_DATA_BUFFER_BINDING_POINT 1
    //! \brief The binding point for the \a hi_z_data buffer.
#define HI_Z_DATA_BUFFER_BINDING_POINT 6
    //! \brief The binding point for the \a gtao_data buffer.
#define GTAO_DATA_BUFFER_BINDING_POINT 2

    //! \brief The vertex input binding point for the position vertex attribute.
#define VERTEX_INPUT_POSITION 0
    //! \brief The vertex input binding point for the normal vertex attribute.
#define VERTEX_INPUT_NORMAL 1
    //! \brief The vertex input binding point for the uv vertex attribute.
#define VERTEX_INPUT_TEXCOORD 2
    //! \brief The vertex input binding point for the tangent vertex attribute.
#define VERTEX_INPUT_TANGENT 3

    //! \brief The target binding point for the gbuffer color attachment 0.
#define GBUFFER_OUTPUT_TARGET0 0
    //! \brief The target binding point for the gbuffer color attachment 1.
#define GBUFFER_OUTPUT_TARGET1 1
    //! \brief The target binding point for the gbuffer color attachment 2.
#define GBUFFER_OUTPUT_TARGET2 2
    //! \brief The target binding point for the gbuffer color attachment 3.
#define GBUFFER_OUTPUT_TARGET3 3

    //! \brief The sampler and texture binding point for the gbuffer color attachment 0.
#define GBUFFER_TEXTURE_SAMPLER_TARGET0 0
    //! \brief The sampler and texture binding point for the gbuffer color attachment 1.
#define GBUFFER_TEXTURE_SAMPLER_TARGET1 1
    //! \brief The sampler and texture binding point for the gbuffer color attachment 2.
#define GBUFFER_TEXTURE_SAMPLER_TARGET2 2
    //! \brief The sampler and texture binding point for the gbuffer color attachment 3.
#define GBUFFER_TEXTURE_SAMPLER_TARGET3 3
    //! \brief The sampler and texture binding point for the gbuffer depth stencil attachment.
#define GBUFFER_TEXTURE_SAMPLER_DEPTH 4

    //! \brief The sampler and texture binding point for the geometry base color texture.
#define GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR 0
    //! \brief The sampler and texture binding point for the geometry roughness, metallic (, occlusion) texture.
#define GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC 1
    //! \brief The sampler and texture binding point for the geometry occlusion texture.
#define GEOMETRY_TEXTURE_SAMPLER_OCCLUSION 2
    //! \brief The sampler and texture binding point for the geometry normal texture.
#define GEOMETRY_TEXTURE_SAMPLER_NORMAL 3
    //! \brief The sampler and texture binding point for the geometry emissive color texture.
#define GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR 4

    //! \brief The sampler and texture binding point for the image based lighting irradiance map.
#define IBL_SAMPLER_IRRADIANCE_MAP 5
    //! \brief The sampler and texture binding point for the image based lighting radiance map.
#define IBL_SAMPLER_RADIANCE_MAP 6
    //! \brief The sampler and texture binding point for the image based lighting lookup table texture.
#define IBL_SAMPLER_LOOKUP 7
    //! \brief The samplerShadow and texture binding point for the shadow map.
#define SAMPLER_SHADOW_SHADOW_MAP 8
    //! \brief The sampler and texture binding point for the shadow map.
#define SAMPLER_SHADOW_MAP 9

    //! \brief The sampler and texture binding point for the target color hdr attachment to compose.
#define COMPOSING_HDR_SAMPLER 0
    //! \brief The sampler and texture binding point for the target depth attachment to pass through.
#define COMPOSING_DEPTH_SAMPLER 1

    //! \brief The image binding point for the output target color hdr attachment to compute the average luminance for.
#define HDR_IMAGE_LUMINANCE_COMPUTE 0

    //! \brief The sampler and texture binding point for the depth texture to sample from (first pass original depth, afterwards RG texture from previous pass)
#define HI_Z_DEPTH_SAMPLER 0
    //! \brief The image binding point for the output target color attachment to write the min/max depth to.
#define HI_Z_IMAGE_COMPUTE 1

} // namespace mango

#endif // MANGO_RENDERER_BINDINGS_HPP