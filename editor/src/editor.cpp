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
    if (!c.lock())
    {
        printf("Context is expired!");
        return false;
    }

    return true;
}

void editor::update(float dt)
{
    MANGO_UNUSED(dt);
}

void editor::destroy() {}
