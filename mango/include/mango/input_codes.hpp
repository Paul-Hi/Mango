//! \file      input_codes.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_INPUT_CODES_HPP
#define MANGO_INPUT_CODES_HPP

#include <mango/types.hpp>

namespace mango
{
    //! \brief Possible key codes.
    //! \details Actually glfw key codes for now. Just unknown is 0 instead of -1.
    enum class key_code : uint32
    {
        key_unknown       = 0,
        key_space         = 32,
        key_apostrophe    = 39,
        key_comma         = 44,
        key_minus         = 45,
        key_period        = 46,
        key_slash         = 47,
        key_0             = 48,
        key_1             = 49,
        key_2             = 50,
        key_3             = 51,
        key_4             = 52,
        key_5             = 53,
        key_6             = 54,
        key_7             = 55,
        key_8             = 56,
        key_9             = 57,
        key_semicolon     = 59,
        key_equal         = 61,
        key_a             = 65,
        key_b             = 66,
        key_c             = 67,
        key_d             = 68,
        key_e             = 69,
        key_f             = 70,
        key_g             = 71,
        key_h             = 72,
        key_i             = 73,
        key_j             = 74,
        key_k             = 75,
        key_l             = 76,
        key_m             = 77,
        key_n             = 78,
        key_o             = 79,
        key_p             = 80,
        key_q             = 81,
        key_r             = 82,
        key_s             = 83,
        key_t             = 84,
        key_u             = 85,
        key_v             = 86,
        key_w             = 87,
        key_x             = 88,
        key_y             = 89,
        key_z             = 90,
        key_left_bracket  = 91,
        key_backslash     = 92,
        key_right_bracket = 93,
        key_grave_accent  = 96,
        key_world_1       = 161,
        key_world_2       = 162,
        key_escape        = 256,
        key_enter         = 257,
        key_tab           = 258,
        key_backspace     = 259,
        key_insert        = 260,
        key_delete        = 261,
        key_right         = 262,
        key_left          = 263,
        key_down          = 264,
        key_up            = 265,
        key_page_up       = 266,
        key_page_down     = 267,
        key_home          = 268,
        key_end           = 269,
        key_caps_lock     = 280,
        key_scroll_lock   = 281,
        key_num_lock      = 282,
        key_print_screen  = 283,
        key_pause         = 284,
        key_f1            = 290,
        key_f2            = 291,
        key_f3            = 292,
        key_f4            = 293,
        key_f5            = 294,
        key_f6            = 295,
        key_f7            = 296,
        key_f8            = 297,
        key_f9            = 298,
        key_f10           = 299,
        key_f11           = 300,
        key_f12           = 301,
        key_f13           = 302,
        key_f14           = 303,
        key_f15           = 304,
        key_f16           = 305,
        key_f17           = 306,
        key_f18           = 307,
        key_f19           = 308,
        key_f20           = 309,
        key_f21           = 310,
        key_f22           = 311,
        key_f23           = 312,
        key_f24           = 313,
        key_f25           = 314,
        key_kp_0          = 320,
        key_kp_1          = 321,
        key_kp_2          = 322,
        key_kp_3          = 323,
        key_kp_4          = 324,
        key_kp_5          = 325,
        key_kp_6          = 326,
        key_kp_7          = 327,
        key_kp_8          = 328,
        key_kp_9          = 329,
        key_kp_decimal    = 330,
        key_kp_divide     = 331,
        key_kp_multiply   = 332,
        key_kp_subtract   = 333,
        key_kp_add        = 334,
        key_kp_enter      = 335,
        key_kp_equal      = 336,
        key_left_shift    = 340,
        key_left_control  = 341,
        key_left_alt      = 342,
        key_left_super    = 343,
        key_right_shift   = 344,
        key_right_control = 345,
        key_right_alt     = 346,
        key_right_super   = 347,
        key_menu          = 348,
        count
    };

    //! \brief Possible mouse button identifiers.
    //! \details Actually glfw mouse button codes for now.
    enum class mouse_button : uint8
    {
        mouse_button_1      = 0,
        mouse_button_2      = 1,
        mouse_button_3      = 2,
        mouse_button_4      = 3,
        mouse_button_5      = 4,
        mouse_button_6      = 5,
        mouse_button_7      = 6,
        mouse_button_8      = 7,
        mouse_button_left   = mouse_button_1,
        mouse_button_right  = mouse_button_2,
        mouse_button_middle = mouse_button_3,
        count               = 8
    };

    //! \brief Possible actions for input.
    //! \details Actually action from glfw for now.
    enum class input_action : uint8
    {
        release = 0,
        press   = 1,
        repeat  = 2
    };

    //! \brief Possible modifier keys.
    //! \details Actually modifiers from glfw for now.
    enum class modifier : uint8
    {
        none               = 0,
        modifier_shift     = 1 << 0,
        modifier_control   = 1 << 1,
        modifier_alt       = 1 << 2,
        modifier_super     = 1 << 3,
        modifier_caps_lock = 1 << 4,
        modifier_num_lock  = 1 << 5
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(modifier)

    //
    // Display callbacks.
    //

    //! \brief Type definition of a callback for \a display position changes.
    using display_position_callback = std::function<void(int32 x_position, int32 y_position)>;
    //! \brief Type definition of a callback for \a display size changes.
    using display_resize_callback = std::function<void(int32 width, int32 height)>;
    //! \brief Type definition of a callback for \a display close events.
    using display_close_callback = std::function<void()>;
    //! \brief Type definition of a callback for \a display refresh events.
    using display_refresh_callback = std::function<void()>;
    //! \brief Type definition of a callback \a display focus events.
    using display_focus_callback = std::function<void(bool focused)>;
    //! \brief Type definition of a callback \a display iconify events.
    using display_iconify_callback = std::function<void(bool iconified)>;
    //! \brief Type definition of a callback \a display maximize events.
    using display_maximize_callback = std::function<void(bool maximized)>;
    //! \brief Type definition of a callback for \a display framebuffer size changes.
    using display_framebuffer_resize_callback = std::function<void(int32 width, int32 height)>;
    //! \brief Type definition of a callback for \a display content scale changes.
    using display_content_scale_callback = std::function<void(float x_scale, float y_scale)>;

    //
    // Input callbacks.
    //

    //! \brief Type definition of a callback for mouse button changes.
    using mouse_button_callback = std::function<void(mouse_button button, input_action action, modifier mods)>;
    //! \brief Type definition of a callback for cursor position changes.
    using cursor_position_callback = std::function<void(double x_position, double y_position)>;
    //! \brief Type definition of a callback for cursor enter events.
    using cursor_enter_callback = std::function<void(bool entered)>;
    //! \brief Type definition of a callback for mouse scrolling.
    using scroll_callback = std::function<void(double x_offset, double y_offset)>;
    //! \brief Type definition of a callback for key changes.
    using key_callback = std::function<void(key_code key, input_action action, modifier mods)>;
    //! \brief Type definition of a callback for dropped paths.
    using drop_callback = std::function<void(int count, const char** paths)>;

} // namespace mango

#endif // MANGO_INPUT_CODES_HPP