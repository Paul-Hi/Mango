//! \file      renderer_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <mango/profile.hpp>
#include <rendering/renderer_impl.hpp>

using namespace mango;

renderer_impl::renderer_impl(const renderer_configuration&, const shared_ptr<context_impl>& context)
    : m_shared_context(context)
{
}

renderer_impl::~renderer_impl() {}
