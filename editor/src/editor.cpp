//! \file      editor.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include "editor.hpp"

MANGO_DEFINE_APPLICATION_MAIN(editor)

editor::editor()
{
    weak_ptr<context> c = get_context();
    if (auto sp = c.lock())
    {
        printf("Context is accessible!");
    }
    else
    {
        printf("Context is expired!");
    }
}