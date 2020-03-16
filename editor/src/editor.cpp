//! \file      editor.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include "editor.hpp"

using namespace mango;

MANGO_DEFINE_APPLICATION_MAIN(editor)

bool editor::create()
{
    weak_ptr<context> c = get_context();
    MANGO_ASSERT(c.lock(), "Context is expired!");

    return true;
}

void editor::update(float dt)
{
    MANGO_UNUSED(dt);
}

void editor::destroy() {}
