//! \file      gl_vertex_array_cache.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GL_VERTEX_ARRAY_CACHE_HPP
#define MANGO_GL_VERTEX_ARRAY_CACHE_HPP

#include <graphics/opengl/gl_graphics_resources.hpp>

namespace mango
{
    //! \brief Cache for opengl vertex arrays used internally.
    class gl_vertex_array_cache
    {
      public:
        gl_vertex_array_cache();
        ~gl_vertex_array_cache();

        //! \brief Returns the \a gl_handle of a specific gl vertex array for given input description.
        //! \details Creates and caches gl vertex arrays.
        //! \param[in] desc The \a vertex_array_data_descriptor specifying the vertex input.
        //! \return The \a gl_handle of a specific gl vertex array for given input description.
        gl_handle get_vertex_array(const vertex_array_data_descriptor& desc);

        //! \brief Returns the \a gl_handle of an empty gl vertex array.
        //! \return The \a gl_handle of an empty gl vertex array.
        gl_handle get_empty_vertex_array();

        // TODO Paul: Invalidate?
      private:
        //! \brief The maximum number of vertex buffer attachments.
        static const int32 max_attached_vertex_buffers = 16; // TODO Paul: Query max vertex buffers. GL_MAX_VERTEX_ATTRIB_BINDINGS

        //! \brief Key for caching vertex arrays.
        struct vertex_array_key
        {
            struct
            {
                //! \brief The \a gfx_uid of the \a gfx_buffer bound as vertex buffer.
                gfx_uid uid = invalid_uid;
                //! \brief The offset in the \a gfx_buffer bound as vertex buffer.
                int32 offset = 0;

            } vertex_buffers[max_attached_vertex_buffers]; //!< All vertex buffers bound to the vertex array.

            //! \brief Bitmask to make comparison and caching more performant.
            int16 binding_bitmask = 0;

            //! \brief The \a gfx_uid of the \a gfx_buffer bound as index buffer.
            gfx_uid index_buffer = invalid_uid;

            //! \brief Comparison operator equal.
            //! \param other The other \a vertex_array_key.
            //! \return True if other \a vertex_array_key is equal to the current one, else false.
            bool operator==(const vertex_array_key& other) const
            {
                if (binding_bitmask != other.binding_bitmask)
                    return false;
                if (index_buffer != other.index_buffer)
                    return false;

                int16 bb  = binding_bitmask;
                int32 idx = 0;
                while (bb)
                {
                    if (bb & 1)
                    {
                        if (vertex_buffers[idx].uid != other.vertex_buffers[idx].uid)
                            return false;
                        if (vertex_buffers[idx].offset != other.vertex_buffers[idx].offset)
                            return false;
                        idx++;
                    }
                    bb >>= 1;
                }

                return true;
            }
        };

        //! \brief Hash for \a vertex_array_keys.
        struct vertex_array_key_hash
        {
            //! \brief Function call operator.
            //! \details Hashes the \a vertex_array_key.
            //! \param[in] k The \a vertex_array_key to hash.
            //! \return The hash for the given \a vertex_array_key.
            std::size_t operator()(const vertex_array_key& k) const
            {
                // https://stackoverflow.com/questions/1646807/quick-and-simple-hash-code-combinations/

                size_t res = 17;
                res        = res * 31 + std::hash<int32>()(k.binding_bitmask);
                res        = res * 31 + std::hash<int64>()(k.index_buffer);

                int16 bb  = k.binding_bitmask;
                int32 idx = 0;
                while (bb)
                {
                    if (bb & 1)
                    {
                        res = res * 31 + std::hash<int64>()(k.vertex_buffers[idx].uid);
                        res = res * 31 + std::hash<int32>()(k.vertex_buffers[idx].offset);
                        idx++;
                    }
                    bb >>= 1;
                }

                return res;
            };
        };

        //! \brief Info to create vertex arrays.
        struct vao_create_info
        {
            struct
            {
                //! \brief The \a gl_handle of the bound vertex buffer.
                gl_handle handle = 0; // No object is zero.
                //! \brief The offset in the bound vertex buffer.
                int32 offset;
                //! \brief The stride of the bound vertex buffer.
                int32 stride;
                //! \brief The \a gfx_vertex_input_rate of the bound vertex buffer.
                gfx_vertex_input_rate input_rate;
            } vertex_buffers[max_attached_vertex_buffers]; //!< All vertex buffers bound to the vertex array.
            //! \brief The \a gl_handle of the bound index buffer.
            gl_handle index_buffer_handle = 0; // No object is zero.
        };

        //! \brief Creates a framebuffer and returns th handle from opengl.
        //! \param[in] create_info The \a framebuffer_create_info used for creation.
        //! \param[in] input_descriptor The \a vertex_input_descriptor describing the layout of the vertex attributes.
        //! \return The \a gl_handle of the created opengl framebuffer.
        gl_handle create(const vao_create_info& create_info, const vertex_input_descriptor* input_descriptor);

        //! \brief The cache mapping \a vertex_array_keys to \a gl_handles of opengl vertex arrays.
        std::unordered_map<vertex_array_key, gl_handle, vertex_array_key_hash> cache;
    };
} // namespace mango

#endif // MANGO_GL_VERTEX_ARRAY_CACHE_HPP
