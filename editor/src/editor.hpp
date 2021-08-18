//! \file      editor.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
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
    mango::sid m_main_camera_node_id;

    mango::display_handle m_main_display;
    mango::renderer_handle m_main_renderer;
    mango::ui_handle m_main_ui;
    mango::scene_handle m_current_scene;

    //! \brief Last mouse position.
    mango::vec2 m_last_mouse_position;
    //! \brief Offsets for camera rotation.
    mango::vec2 m_camera_rotation;
    //! \brief Offsets for camera rotation.
    mango::vec2 m_target_offset;
    //! \brief Radius for camera.
    float m_camera_radius;
};

#endif // EDITOR_HPP
