//! \file      mango.hpp
//! This is the main file included by all applications that use the Mango graphics engine.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
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
#include <mango/display.hpp>
#include <mango/display_event_handler.hpp>
#include <mango/imgui_helper.hpp>
#include <mango/input_codes.hpp>
#include <mango/log.hpp>
// #include <mango/mesh_factory.hpp>
#include <mango/profile.hpp>
#include <mango/renderer.hpp>
#include <mango/resource_structures.hpp>
#include <mango/resources.hpp>
#include <mango/scene.hpp>
#include <mango/scene_structures.hpp>
#include <mango/types.hpp>
#include <mango/ui.hpp>

// Third Party
#include "imgui.h"
#include "tinyfiledialogs.h"

#endif // MANGO_HPP
