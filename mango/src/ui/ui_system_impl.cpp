//! \file      ui_system_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <ui/ui_system_impl.hpp>

using namespace mango;

ui_system_impl::ui_system_impl(const shared_ptr<context_impl>& context)
{
    MANGO_UNUSED(context);
}

ui_system_impl::~ui_system_impl()
{

}

bool ui_system_impl::create()
{
    // Initialize ImGui
    return true;
}

void ui_system_impl::configure(const ui_configuration& configuration)
{
    MANGO_UNUSED(configuration);
}

void ui_system_impl::update(float dt)
{
    MANGO_UNUSED(dt);
}

void ui_system_impl::destroy()
{

}

void ui_system_impl::draw_ui()
{

}
