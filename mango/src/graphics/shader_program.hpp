//! \file      shader_program.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SHADER_PROGRAM_HPP
#define MANGO_SHADER_PROGRAM_HPP

#include <graphics/graphics_object.hpp>
#include <unordered_map>

namespace mango
{
    //! \brief A structure used to store information about uniform bindings.
    struct uniform_binding_data
    {
        //! \brief Information for uniforms.
        struct uniform
        {
            //! \brief The \a unifrom type.
            shader_resource_type type;
        };

        //! \brief A list of \a uniform information filled after compiling.
        //! \details Maps locations to \a uniforms.
        std::unordered_map<int32, uniform> listed_data;
    };

    //! \brief A program containing compiled and linked shaders.
    class shader_program : public graphics_object
    {
      public:
        //! \brief Creates a new \a shader_program describing a graphics pipeline and returns it as a pointer.
        //! \param[in] vertex_shader A pointer to the vertex shader source.
        //! \param[in] tess_control_shader A pointer to the tesselation control shader source.
        //! \param[in] tess_eval_shader A pointer to the tesselation evaluation shader source.
        //! \param[in] geometry_shader A pointer to the geometry shader source.
        //! \param[in] fragment_shader A pointer to the fragment shader source.
        //! \return A pointer to the new \a shader_program.
        static shader_program_ptr create_graphics_pipeline(shader_ptr vertex_shader, shader_ptr tess_control_shader, shader_ptr tess_eval_shader, shader_ptr geometry_shader, shader_ptr fragment_shader);

        //! \brief Creates a new \a shader_program describing a graphics pipeline and returns it as a pointer.
        //! \param[in] compute_shader A pointer to the compute shader source.
        //! \return A pointer to the new \a shader_program.
        static shader_program_ptr create_compute_pipeline(shader_ptr compute_shader);

        //! \brief Retrieves the binding data for the \a shader_program.
        //! \details For this to work there has to be a consistence in the uniform locations.
        //! \return The \a uniform_binding_data of all \a shaders in the \a shader_program.
        virtual const uniform_binding_data& get_single_bindings() = 0;

      protected:
        shader_program() = default;
        ~shader_program() = default;
    };
} // namespace mango

#endif // MANGO_SHADER_PROGRAM_HPP
