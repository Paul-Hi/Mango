//! \file      imgui_helper.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_IMGUI_HELPER_HPP
#define MANGO_IMGUI_HELPER_HPP

#include <imgui.h>
#include <imgui_internal.h>
#include <mango/types.hpp>

namespace mango
{
    //! \brief Splits the current imgui window in multiple columns.
    //! \param[in] string_id The id for identification. Used by imgui.
    //! \param[in] number The number of columns to split into.
    //! \param[in] column_width The width of the first column.
    void column_split(const string& string_id, int32 number = 2, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

    //! \brief Moves to the next column.
    //! \details Loops around. Should be called between column_split and column_merge.
    void column_next();

    //! \brief Merges the columns back into one.
    void column_merge();

    //! \brief Prints a wrapped text, calculating the available width.
    void text_wrapped(const string& text);

    //! \brief Draws a custom information with mangos ui alignments.
    //! \param[in] label The label of the information.
    //! \param[in] component_function A function callback containing imgui code to specify the information.
    //! \param[in] group_width_modifier Modifier to increase/decrease the item width for multiple grouped items with spacing.
    //! \param[in] column_width The width of the first column.
    void custom_info(const string& label, std::function<void()> component_function, float group_width_modifier = 0.0f, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

    //! \brief Draws a custom aligned value with mangos ui alignments.
    //! \brief The difference to custom_info(...) is, that this one has a component_function with reset possibility and returns the value change.
    //! \param[in] label The label of the value.
    //! \param[in] component_function A function callback containing imgui code to specify the value. Also gets a "reset" argument.
    //! \param[in] group_width_modifier Modifier to increase/decrease the item width for multiple grouped items with spacing.
    //! \param[in] column_width The width of the first column.
    //! \return True, when value changed, else false.
    bool custom_aligned(const string& label, std::function<bool(bool reset)> component_function, float group_width_modifier = 0.0f, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

    //! \brief Draws n float drag sliders with mangos ui alignments.
    //! \param[in] label The label of the value/s.
    //! \param[in,out] values A pointer to the n floating point values.
    //! \param[in] components The number of components in values.
    //! \param[in] reset_value Pointer to n reset values.
    //! \param[in] speed The change speed when dragging.
    //! \param[in] min_value The minimum possible value.
    //! \param[in] max_value The maximum possible value.
    //! \param[in] format The format specification for displaying the float number.
    //! \param[in] component_buttons True if { "X", "Y", "Z", "W", "" } should be displayed before drag sliders, else false.
    //! \param[in] column_width The width of the first column.
    //! \return True, when value/s changed, else false.
    bool drag_float_n(const string& label, float* values, int32 components, float* reset_value, float speed = 1.0f, float min_value = 0.0f, float max_value = 0.0f, const char* format = "%.3f",
                      bool component_buttons = false, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

    //! \brief Draws n float sliders with mangos ui alignments.
    //! \param[in] label The label of the value/s.
    //! \param[in,out] values A pointer to the n floating point values.
    //! \param[in] components The number of components in values.
    //! \param[in] reset_value Pointer to n reset values.
    //! \param[in] min_value The minimum possible value.
    //! \param[in] max_value The maximum possible value.
    //! \param[in] format The format specification for displaying the float number.
    //! \param[in] component_buttons True if { "X", "Y", "Z", "W", "" } should be displayed before sliders, else false.
    //! \param[in] column_width The width of the first column.
    //! \return True, when value/s changed, else false.
    bool slider_float_n(const string& label, float* values, int32 components, float* reset_value, float min_value = 0.0f, float max_value = 0.0f, const char* format = "%.3f",
                        bool component_buttons = false, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

    //! \brief Draws n integer sliders with mangos ui alignments.
    //! \param[in] label The label of the value/s.
    //! \param[in,out] values A pointer to the n integer values.
    //! \param[in] components The number of components in values.
    //! \param[in] reset_value Pointer to n reset values.
    //! \param[in] min_value The minimum possible value.
    //! \param[in] max_value The maximum possible value.
    //! \param[in] format The format specification for displaying the integer number.
    //! \param[in] component_buttons True if { "X", "Y", "Z", "W", "" } should be displayed before sliders, else false.
    //! \param[in] column_width The width of the first column.
    //! \return True, when value/s changed, else false.
    bool slider_int_n(const string& label, int32* values, int32 components, int32* reset_value, int32 min_value = 0, int32 max_value = 0, const char* format = "%d", bool component_buttons = false,
                      float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

    //! \brief Draws a rgb or rgba color edit with mangos ui alignments. Range is 0.0 - 1.0.
    //! \param[in] label The label of the value/s.
    //! \param[in,out] values A pointer to the n floating point values.
    //! \param[in] components The number of components in values.
    //! \param[in] reset_value Pointer to 3(4) reset values.
    //! \param[in] column_width The width of the first column.
    //! \return True, when value/s changed, else false.
    bool color_edit(const string& label, float* values, int32 components, float* reset_value, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

    //! \brief Draws a checkbox with mangos ui alignments.
    //! \param[in] label The label of the value/s.
    //! \param[in,out] value Pointer to the value.
    //! \param[in] reset_value The reset value.
    //! \param[in] column_width The width of the first column.
    //! \return True, when value changed, else false.
    bool checkbox(const string& label, bool* value, bool reset_value, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

    //! \brief Draws a combo with mangos ui alignments.
    //! \param[in] label The label of the combo.
    //! \param[in,out] list Value list. List of char*.
    //! \param[in] size The number of values in list.
    //! \param[in] current_idx The current index that is selected in the combo.
    //! \param[in] reset_value The reset index.
    //! \param[in] column_width The width of the first column.
    //! \return True, when current_idx changed, else false.
    bool combo(const string& label, const char** list, int32 size, int32& current_idx, int32 reset_value, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

    //! \brief Draws an image box with loading functionality with mangos ui alignments.
    //! \param[in] label The label of the texture value.
    //! \param[in] texture_native_handle The native handle of the texture.
    //! \param[in] size The width and height of the image box
    //! \param[out] load_new Is true if a new image should be loaded afterwards, else false.
    //! \param[in] column_width The width of the first column.
    //! \return True, when value changed, else false.
    bool image_load(const string& label, void* texture_native_handle, const vec2& size, bool& load_new, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

} // namespace mango

#endif // MANGO_IMGUI_HELPER_HPP
