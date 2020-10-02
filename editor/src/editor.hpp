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
        return "Mango Editor";
    }

  private:
    //! \brief Main camera. Testing.
    mango::entity m_main_camera;

    //! \brief Last mouse position.
    glm::vec2 m_last_mouse_position;
    //! \brief Offsets for camera rotation.
    glm::vec2 m_camera_rotation;
    //! \brief Offsets for camera rotation.
    glm::vec2 m_target_offset;
    //! \brief Radius for camera.
    float m_camera_radius;
};

#endif // EDITOR_HPP
