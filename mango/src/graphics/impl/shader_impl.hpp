//! \file      shader_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SHADER_IMPL_HPP
#define MANGO_SHADER_IMPL_HPP

#include <graphics/shader.hpp>

namespace mango
{
    //! \brief The implementation of the \a shader.
    class shader_impl : public shader
    {
      public:
        //! \brief Constructs the \a shader_impl.
        //! \param[in] configuration The \a shader_configuration.
        shader_impl(const shader_configuration& configuration);
        ~shader_impl();

        //! \brief Returns the \a shader_type of the \a shader.
        //! \return Type of the \a shader.
        inline shader_type get_type() override
        {
            return m_type;
        }

      private:
        //! \brief Path to shader source of this \a shader. Relative to project folder.
        const char* m_path;
        //! \brief The \a shader_type of this \a shader.
        shader_type m_type;
    };
} // namespace mango

#endif // MANGO_SHADER_IMPL_HPP
