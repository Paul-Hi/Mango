//! \file      gl_shader_program_cache.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GL_SHADER_PROGRAM_CACHE_HPP
#define MANGO_GL_SHADER_PROGRAM_CACHE_HPP

#include <graphics/opengl/gl_graphics_resources.hpp>

namespace mango
{
    //! \brief Cache for opengl shader programs used internally.
    class gl_shader_program_cache
    {
      public:
        gl_shader_program_cache();
        ~gl_shader_program_cache();

        //! \brief Returns the \a gl_handle of a specific gl shader program for a given \a graphics_shader_stage_descriptor.
        //! \details Creates and caches gl shader programs for given stages.
        //! \param[in] desc The \a graphics_shader_stage_descriptor to use for creating the shader program.
        //! \return The \a gl_handle of a specific gl shader program for a given \a graphics_shader_stage_descriptor.
        gl_handle get_shader_program(const graphics_shader_stage_descriptor& desc);

        //! \brief Returns the \a gl_handle of a specific gl shader program for a given \a compute_shader_stage_descriptor.
        //! \details Creates and caches gl shader programs for given stages.
        //! \param[in] desc The \a compute_shader_stage_descriptor to use for creating the shader program.
        //! \return The \a gl_handle of a specific gl shader program for a given \a compute_shader_stage_descriptor.
        gl_handle get_shader_program(const compute_shader_stage_descriptor& desc);

        // TODO Paul: Invalidate?
      private:
        //! \brief The maximum number of shader stages.
        static const int32 max_shader_stages = 5; // For now // TODO Paul: Get HW capabilities ...

        //! \brief Key for caching shader programs.
        struct shader_program_key
        {
            //! \brief The number of shader stages linked to the program.
            int32 stage_count;
            //! \brief The types of all linked shader stages.
            gfx_shader_stage_type stage_types[max_shader_stages];
            //! \brief The \a gfx_uids for the \a gfx_shader_stages attached.
            gfx_uid shader_stage_uids[max_shader_stages];

            //! \brief Comparison operator equal.
            //! \param other The other \a shader_program_key.
            //! \return True if other \a shader_program_key is equal to the current one, else false.
            bool operator==(const shader_program_key& other) const
            {
                if (stage_count != other.stage_count)
                    return false;

                for (int32 i = 0; i < stage_count; ++i)
                {
                    if (stage_types[i] != other.stage_types[i])
                        return false;
                    if (shader_stage_uids[i] != other.shader_stage_uids[i])
                        return false;
                }

                return true;
            }
        };

        //! \brief Hash for \a shader_program_keys.
        struct shader_program_key_hash
        {
            //! \brief Function call operator.
            //! \details Hashes the \a shader_program_key.
            //! \param[in] k The \a shader_program_key to hash.
            //! \return The hash for the given \a shader_program_key.
            std::size_t operator()(const shader_program_key& k) const
            {
                // https://stackoverflow.com/questions/1646807/quick-and-simple-hash-code-combinations/

                size_t res = 17;
                res        = res * 31 + std::hash<int32>()(k.stage_count);

                for (int32 i = 0; i < k.stage_count; ++i)
                {
                    res = res * 31 + std::hash<uint8>()(static_cast<uint8>(k.stage_types[i]));
                    res = res * 31 + std::hash<int64>()(k.shader_stage_uids[i]);
                }

                return res;
            };
        };

        //! \brief Creates a shader program and returns th handle from opengl.
        //! \param[in] stage_count The number of active stages.
        //! \param[in] handles The \a gl_handles of the shaders.
        //! \return The \a gl_handle of the created opengl framebuffer.
        gl_handle create(int32 stage_count, gl_handle handles[max_shader_stages]);

        //! \brief The cache mapping \a shader_program_keys to \a gl_handles of opengl shader programs.
        std::unordered_map<shader_program_key, gl_handle, shader_program_key_hash> cache;
    };
} // namespace mango

#endif // MANGO_GL_SHADER_PROGRAM_CACHE_HPP
