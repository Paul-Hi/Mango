//! \file      renderer_pipeline_cache.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_RENDERER_PIPELINE_CACHE_HPP
#define MANGO_RENDERER_PIPELINE_CACHE_HPP

#include <core/context_impl.hpp>

namespace mango
{
    //! \brief Cache for \a gfx_pipelines.
    //! \details Takes basic pipeline create infos, creates and caches \a gfx_pipelines.
    class renderer_pipeline_cache
    {
      public:
        //! \brief Constructs a new \a renderer_pipeline_cache.
        //! \param[in] context The internally shared context of mango.
        renderer_pipeline_cache(const shared_ptr<context_impl>& context)
            : m_shared_context(context){};

        ~renderer_pipeline_cache() = default;

        //! \brief Sets a \a graphics_pipeline_create_info as base for graphics \a gfx_pipelines for opaque geometry.
        //! \param[in] basic_create_info The \a graphics_pipeline_create_info to set.
        inline void set_opaque_base(const graphics_pipeline_create_info& basic_create_info)
        {
            m_opaque_create_info = basic_create_info;
        }
        //! \brief Sets a \a graphics_pipeline_create_info as base for graphics \a gfx_pipelines for transparent geometry.
        //! \param[in] basic_create_info The \a graphics_pipeline_create_info to set.
        inline void set_transparent_base(const graphics_pipeline_create_info& basic_create_info)
        {
            m_transparent_create_info = basic_create_info;
        }
        //! \brief Sets a \a graphics_pipeline_create_info as base for graphics \a gfx_pipelines for shadow pass geometry.
        //! \param[in] basic_create_info The \a graphics_pipeline_create_info to set.
        inline void set_shadow_base(const graphics_pipeline_create_info& basic_create_info)
        {
            m_shadow_create_info = basic_create_info;
        }

        //! \brief Gets a graphics \a gfx_pipeline for opaque geometry.
        //! \param[in] geo_vid The \a vertex_input_descriptor of the geometry.
        //! \param[in] geo_iad The \a input_assembly_dedscriptor of the geometry.
        //! \param[in] wireframe True if the pipeline should render wireframe, else false.
        //! \return A \a gfx_handle of a \a gfx_pipeline to use for rendering opaque geometry.
        gfx_handle<const gfx_pipeline> get_opaque(const vertex_input_descriptor& geo_vid, const input_assembly_descriptor& geo_iad, bool wireframe);
        //! \brief Gets a graphics \a gfx_pipeline for transparent geometry.
        //! \param[in] geo_vid The \a vertex_input_descriptor of the geometry.
        //! \param[in] geo_iad The \a input_assembly_dedscriptor of the geometry.
        //! \param[in] wireframe True if the pipeline should render wireframe, else false.
        //! \return A \a gfx_handle of a \a gfx_pipeline to use for rendering transparent geometry.
        gfx_handle<const gfx_pipeline> get_transparent(const vertex_input_descriptor& geo_vid, const input_assembly_descriptor& geo_iad, bool wireframe);
        //! \brief Gets a graphics \a gfx_pipeline for shadow pass geometry.
        //! \param[in] geo_vid The \a vertex_input_descriptor of the geometry.
        //! \param[in] geo_iad The \a input_assembly_dedscriptor of the geometry.
        //! \return A \a gfx_handle of a \a gfx_pipeline to use for rendering shadow pass geometry.
        gfx_handle<const gfx_pipeline> get_shadow(const vertex_input_descriptor& geo_vid, const input_assembly_descriptor& geo_iad);

      private:
        //! \brief Key for caching \a gfx_pipelines.
        struct pipeline_key
        {
            //! \brief The \a vertex_input_descriptor of the \a pipeline_key.
            vertex_input_descriptor vid;
            //! \brief The \a input_assembly_descriptor of the \a pipeline_key.
            input_assembly_descriptor iad;
            //! \brief True if the cached pipeline renders wireframe, else false.
            bool wireframe;

            //! \brief Comparison operator equal.
            //! \param other The other \a pipeline_key.
            //! \return True if other \a pipeline_key is equal to the current one, else false.
            bool operator==(const pipeline_key& other) const
            {
                if (wireframe != other.wireframe)
                    return false;

                if (vid.binding_description_count != other.vid.binding_description_count)
                    return false;
                // attribute description count == binding description count ---> ALWAYS
                if (iad.topology != other.iad.topology)
                    return false;

                for (int32 i = 0; i < vid.binding_description_count; ++i)
                {
                    auto& bd0 = vid.binding_descriptions[i];
                    auto& bd1 = other.vid.binding_descriptions[i];

                    if (bd0.binding != bd1.binding)
                        return false;
                    if (bd0.input_rate != bd1.input_rate)
                        return false;
                    if (bd0.stride != bd1.stride)
                        return false;

                    auto& ad0 = vid.attribute_descriptions[i];
                    auto& ad1 = other.vid.attribute_descriptions[i];

                    if (ad0.binding != ad1.binding)
                        return false;
                    if (ad0.attribute_format != ad1.attribute_format)
                        return false;
                    if (ad0.location != ad1.location)
                        return false;
                    if (ad0.offset != ad1.offset)
                        return false;
                }

                return true;
            }
        };

        //! \brief Hash for \a pipeline_keys.
        struct pipeline_key_hash
        {
            //! \brief Function call operator.
            //! \details Hashes the \a pipeline_key.
            //! \param[in] k The \a pipeline_key to hash.
            //! \return The hash for the given \a pipeline_key.
            std::size_t operator()(const pipeline_key& k) const
            {
                // https://stackoverflow.com/questions/1646807/quick-and-simple-hash-code-combinations/

                size_t res = 17;

                res = res * 31 + std::hash<bool>()(k.wireframe);
                res = res * 31 + std::hash<int32>()(k.vid.binding_description_count);
                res = res * 31 + std::hash<uint8>()(static_cast<uint8>(k.iad.topology));

                for (int32 i = 0; i < k.vid.binding_description_count; ++i)
                {
                    auto& bd = k.vid.binding_descriptions[i];

                    res = res * 31 + std::hash<int32>()(bd.binding);
                    res = res * 31 + std::hash<uint8>()(static_cast<uint8>(bd.input_rate));
                    res = res * 31 + std::hash<int32>()(bd.stride);

                    auto& ad = k.vid.attribute_descriptions[i];

                    res = res * 31 + std::hash<int32>()(ad.binding);
                    res = res * 31 + std::hash<uint32>()(static_cast<uint32>(ad.attribute_format));
                    res = res * 31 + std::hash<int32>()(ad.location);
                    res = res * 31 + std::hash<int32>()(ad.offset);
                }

                return res;
            };
        };

        //! \brief The \a graphics_pipeline_create_info used as base for creating \a gfx_pipelines rendering opaque geometry.
        graphics_pipeline_create_info m_opaque_create_info;
        //! \brief The \a graphics_pipeline_create_info used as base for creating \a gfx_pipelines rendering transparent geometry.
        graphics_pipeline_create_info m_transparent_create_info;
        //! \brief The \a graphics_pipeline_create_info used as base for creating \a gfx_pipelines rendering shadow pass geometry.
        graphics_pipeline_create_info m_shadow_create_info;

        //! \brief The cache mapping \a pipeline_keys to \a gfx_pipelines of rendering opaque geometry.
        std::unordered_map<pipeline_key, gfx_handle<const gfx_pipeline>, pipeline_key_hash> m_opaque_cache;
        //! \brief The cache mapping \a pipeline_keys to \a gfx_pipelines of rendering transparent geometry.
        std::unordered_map<pipeline_key, gfx_handle<const gfx_pipeline>, pipeline_key_hash> m_transparent_cache;
        //! \brief The cache mapping \a pipeline_keys to \a gfx_pipelines of rendering shadow pass geometry.
        std::unordered_map<pipeline_key, gfx_handle<const gfx_pipeline>, pipeline_key_hash> m_shadow_cache;

        //! \brief Mangos internal context for shared usage.
        shared_ptr<context_impl> m_shared_context;
    };

} // namespace mango

#endif // MANGO_RENDERER_PIPELINE_CACHE_HPP
