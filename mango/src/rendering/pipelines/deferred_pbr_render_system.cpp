//! \file      deferred_pbr_render_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <rendering/pipelines/deferred_pbr_render_system.hpp>

using namespace mango;

deferred_pbr_render_system::deferred_pbr_render_system() {}

deferred_pbr_render_system::~deferred_pbr_render_system() {}

bool deferred_pbr_render_system::create()
{
    return true;
}

void deferred_pbr_render_system::configure(const render_configuration& configuration)
{
    MANGO_UNUSED(configuration);
}

void deferred_pbr_render_system::start_frame()
{
    // clear command queue
    std::queue<render_command>().swap(m_command_queue);
}

void deferred_pbr_render_system::submit(const render_command& command)
{
    m_command_queue.push(command);
}

void deferred_pbr_render_system::finish_frame() {}

void deferred_pbr_render_system::render() {}

void deferred_pbr_render_system::update(float dt)
{
    MANGO_UNUSED(dt);
}

void deferred_pbr_render_system::destroy() {}

render_pipeline deferred_pbr_render_system::get_base_render_pipeline()
{
    return render_pipeline::deferred_pbr;
}
