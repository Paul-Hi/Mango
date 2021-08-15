//! \file      gl_framebuffer_cache.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GL_FRAMEBUFFER_CACHE_HPP
#define MANGO_GL_FRAMEBUFFER_CACHE_HPP

#include <graphics/opengl/gl_graphics_resources.hpp>

namespace mango
{
    //! \brief Cache for opengl framebuffers used internally.
    class gl_framebuffer_cache
    {
      public:
        gl_framebuffer_cache();
        ~gl_framebuffer_cache();

        //! \brief Returns the \a gl_handle of a specific gl framebuffer for given render targets.
        //! \details Creates and caches gl framebuffers.
        //! \param[in] count The number of render targets.
        //! \param[in] render_targets A list of \a gfx_textures describing the render targets to use.
        //! \param[in] depth_stencil_target The \a gfx_texture describing the depth stencil target.
        //! \return The \a gl_handle of a specific gl framebuffer for given render targets.
        gl_handle get_framebuffer(int32 count, gfx_handle<const gfx_texture>* render_targets, gfx_handle<const gfx_texture> depth_stencil_target);

        //! \brief Prepares the cache to expect a certain request.
        //! \param[in] desc The \a render_output_description with information what to expect.
        void prepare(const render_output_description& desc)
        {
            MANGO_UNUSED(desc);
        }; // TODO Paul: Implement something usefull!
           // TODO Paul: Invalidate?
      private:
        //! \brief The maximum number of render targets/framebuffer attachments.
        static const int32 max_render_targets = 8 + 1; // TODO Paul: Get HW capabilities ...

        //! \brief Key for caching framebuffers.
        struct framebuffer_key
        {
            framebuffer_key()  = default;
            ~framebuffer_key() = default;

            //! \brief The number of attachments.
            int32 attachment_count = 0;
            //! \brief The \a gfx_uids for the \a gfx_textures attached.
            gfx_uid texture_uids[max_render_targets];

            //! \brief Comparison operator equal.
            //! \param other The other \a framebuffer_key.
            //! \return True if other \a framebuffer_key is equal to the current one, else false.
            bool operator==(const framebuffer_key& other) const
            {
                if (attachment_count != other.attachment_count)
                    return false;

                for (int32 i = 0; i < attachment_count; ++i)
                {
                    if (texture_uids[i] != other.texture_uids[i])
                        return false;
                }

                return true;
            }
        };

        //! \brief Hash for \a framebuffer_keys.
        struct framebuffer_key_hash
        {
            //! \brief Function call operator.
            //! \details Hashes the \a framebuffer_key.
            //! \param[in] k The \a framebuffer_key to hash.
            //! \return The hash for the given \a framebuffer_key.
            std::size_t operator()(const framebuffer_key& k) const
            {
                // https://stackoverflow.com/questions/1646807/quick-and-simple-hash-code-combinations/

                size_t res = 17;

                res = res * 31 + std::hash<int32>()(k.attachment_count);

                for (int32 i = 0; i < k.attachment_count; ++i)
                {
                    res = res * 31 + std::hash<int64>()(k.texture_uids[i]);
                }

                return res;
            };
        };

        //! \brief Info to create framebuffers.
        struct framebuffer_create_info
        {
            //! \brief The number of color attachments.
            int32 color_attachments = 0;
            //! \brief The number of depth attachments.
            int32 depth_attachment = 0;
            //! \brief The number of stencil attachments. Pure stencil not supported at the moment.
            int32 stencil_attachment = 0; // TODO Paul: Pure stencil not supported.
            //! \brief The number of depth stencil attachments.
            int32 depth_stencil_attachment = 0;
            //! \brief The \a gl_handles if all attachments.
            gl_handle handles[max_render_targets];
        };

        //! \brief Creates a framebuffer and returns th handle from opengl.
        //! \param[in] create_info The \a framebuffer_create_info used for creation.
        //! \return The \a gl_handle of the created opengl framebuffer.
        gl_handle create(const framebuffer_create_info& create_info);

        //! \brief The cache mapping \a framebuffer_keys to \a gl_handles of opengl framebuffers.
        std::unordered_map<framebuffer_key, gl_handle, framebuffer_key_hash> cache;
    };
} // namespace mango

#endif // MANGO_GL_FRAMEBUFFER_CACHE_HPP
