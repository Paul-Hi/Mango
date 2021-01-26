//! \file      mango.hpp
//! This is the main file included by all applications that use the Mango graphics engine.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_HPP
#define MANGO_HPP

//! \namespace mango This is the main namespace of the Mango graphics engine. Everything should be inside of here.
namespace mango
{
}

#ifdef WIN32
#include <windows.h>
#endif

#include <mango/application.hpp>
#include <mango/assert.hpp>
#include <mango/context.hpp>
#include <mango/imgui_helper.hpp>
#include <mango/input_codes.hpp>
#include <mango/input_system.hpp>
#include <mango/log.hpp>
#include <mango/mesh_factory.hpp>
#include <mango/profile.hpp>
#include <mango/render_system.hpp>
#include <mango/scene.hpp>
#include <mango/scene_component_pool.hpp>
#include <mango/scene_ecs.hpp>
#include <mango/system.hpp>
#include <mango/types.hpp>
#include <mango/ui_system.hpp>
#include <mango/window_system.hpp>

// Third Party
#include "imgui.h"
#include "tinyfiledialogs.h"

#endif // MANGO_HPP
