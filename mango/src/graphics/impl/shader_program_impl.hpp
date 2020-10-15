//! \file      shader_program_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SHADER_PROGRAM_IMPL_HPP
#define MANGO_SHADER_PROGRAM_IMPL_HPP

#include <graphics/shader_program.hpp>

namespace mango
{
    //! \brief An implementation of a \a shader_program.
    class shader_program_impl : public shader_program
    {
      public:
        shader_program_impl();
        ~shader_program_impl();

        const uniform_binding_data& get_single_bindings() override;

        //! \brief Initializes a graphics pipeline.
        //! \param[in] vertex_shader A pointer to the vertex shader source.
        //! \param[in] tess_control_shader A pointer to the tesselation control shader source.
        //! \param[in] tess_eval_shader A pointer to the tesselation evaluation shader source.
        //! \param[in] geometry_shader A pointer to the geometry shader source.
        //! \param[in] fragment_shader A pointer to the fragment shader source.
        void create_graphics_pipeline_impl(shader_ptr vertex_shader, shader_ptr tess_control_shader, shader_ptr tess_eval_shader, shader_ptr geometry_shader, shader_ptr fragment_shader);

        //! \brief Initializes a compute pipeline.
        //! \param[in] compute_shader A pointer to the compute shader source.
        void create_compute_pipeline_impl(shader_ptr compute_shader);

      private:
        //! \brief The data containing information about uniform bindings.
        uniform_binding_data m_binding_data;

        //! \brief Links the \a shader_program.
        void link_program();

        //! \brief All \a shaders attached to this \a shader_program.
        std::vector<shader_ptr> m_shaders;

    };
} // namespace mango

#endif // MANGO_SHADER_PROGRAM_HPP
