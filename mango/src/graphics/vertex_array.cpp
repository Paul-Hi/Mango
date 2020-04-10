//! \file      vertex_array.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <glad/glad.h>
#include <graphics/impl/vertex_array_impl.hpp>
#include <graphics/vertex_array.hpp>

using namespace mango;

vertex_array_ptr vertex_array::create()
{
    return std::static_pointer_cast<vertex_array>(std::make_shared<vertex_array_impl>());
}
