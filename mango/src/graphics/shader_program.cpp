//! \file      shader_program.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/impl/shader_program_impl.hpp>
#include <graphics/shader_program.hpp>

using namespace mango;

shader_program_ptr shader_program::create_graphics_pipeline(shader_ptr vertex_shader, shader_ptr tess_control_shader, shader_ptr tess_eval_shader, shader_ptr geometry_shader,
                                                            shader_ptr fragment_shader)
{
    auto impl = std::make_shared<shader_program_impl>();
    impl->create_graphics_pipeline_impl(vertex_shader, tess_control_shader, tess_eval_shader, geometry_shader, fragment_shader);
    return std::static_pointer_cast<shader_program>(impl);
}

shader_program_ptr shader_program::create_compute_pipeline(shader_ptr compute_shader)
{
    auto impl = std::make_shared<shader_program_impl>();
    impl->create_compute_pipeline_impl(compute_shader);
    return std::static_pointer_cast<shader_program>(impl);
}
