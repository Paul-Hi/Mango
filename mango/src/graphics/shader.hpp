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
    //! \brief Structure describing a define in a shader.
    struct shader_define
    {
        const char* name; //!< The name of the define.
        const char* value; //!< The value of the define.
    };

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
        //! \param[in] type The type of the shader.
        //! \param[in] defines The defines to inject into the shader.
        shader_configuration(const char* path, shader_type type, std::initializer_list<shader_define> defines)
            : graphics_configuration()
            , path(path)
            , type(type)
            , defines(defines)
        {
        }

        //! \brief Path to shader source. Relative to project folder.
        const char* path = nullptr;

        //! \brief The type of the shader described by the source.
        shader_type type = shader_type::none;

        //! \brief The defines injected into the shader.
        std::vector<shader_define> defines;

        bool is_valid() const
        {
            return nullptr != path && type != shader_type::none;
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
        shader()  = default;
        ~shader() = default;
    };
} // namespace mango

#endif // MANGO_SHADER_HPP
