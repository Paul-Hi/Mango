//! \file      editor.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <mango/mango.hpp>

//! \brief Editor class.
//! \details An application that provides an user interface to create, load, change and save scenes.
class editor : public mango::application
{
  public:
    bool create() override;
    void update(float dt) override;
    void destroy() override;
    const char* get_name() override
    {
        return "Editor";
    }
};

#endif // EDITOR_HPP
