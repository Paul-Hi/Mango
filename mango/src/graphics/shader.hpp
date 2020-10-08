//! \file      shader.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SHADER_HPP
#define MANGO_SHADER_HPP

#include <graphics/graphics_object.hpp>

namespace mango
{
    //! \brief A configuration for \a shaders.
    class shader_configuration : public graphics_configuration
    {
      public:
        //! \brief Constructs a new \a shader_configuration with default values.
        shader_configuration()
            : graphics_configuration()
        {
        }

        //! \brief Constructs a new \a shader_configuration.
        //! \param[in] path The path to the shader source.
        shader_configuration(const char* path)
            : graphics_configuration()
            , m_path(path)
        {
        }

        //! \brief Path to shader source. Relative to project folder.
        const char* m_path;

        //! \brief The type of the shader described by the source.
        shader_type m_type = shader_type::none;

        bool is_valid() const
        {
            return nullptr != m_path && m_type != shader_type::none;
        }
    };

    //! \brief A shader object. Loads source code for gpu programs.
    class shader : public graphics_object
    {
      public:
        //! \brief Creates a new \a shader and returns a pointer to it.
        //! \param[in] configuration The \a shader_configuration for the new \a shader.
        //! \return A pointer to the new \a shader.
        static shader_ptr create(const shader_configuration& configuration);

        //! \brief Returns the \a shader_type of the \a shader.
        //! \return Type of the \a shader.
        virtual shader_type get_type() = 0;

      protected:
        shader() = default;
        ~shader() = default;
    };
} // namespace mango

#endif // MANGO_SHADER_HPP
