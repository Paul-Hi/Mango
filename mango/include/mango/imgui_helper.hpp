//! \file      imgui_helper.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_IMGUI_HELPER_HPP
#define MANGO_IMGUI_HELPER_HPP

#include <imgui.h>
#include <imgui_internal.h>
#include <mango/types.hpp>

namespace mango
{
    void column_split(const string& label, int32 number = 2, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);
    void column_next();
    void column_merge();
    void text_wrapped(const string& label);

    void custom_info(const string& label, std::function<void()> component_function, float group_width_modifier = 0.0f, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);
    bool custom_aligned(const string& label, std::function<bool(bool reset)> component_function, float group_width_modifier = 0.0f, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);
    bool drag_float_n(const string& label, float* values, int32 components, float* reset_value, float speed = 1.0f, float min_value = 0.0f, float max_value = 0.0f, const char* format = "%.3f",
                      bool component_buttons = false, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);
    bool slider_float_n(const string& label, float* values, int32 components, float* reset_value, float min_value = 0.0f, float max_value = 0.0f, const char* format = "%.3f",
                        bool component_buttons = false, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

    bool slider_int_n(const string& label, int32* values, int32 components, int32* reset_value, int32 min_value = 0, int32 max_value = 0, const char* format = "%d", bool component_buttons = false,
                      float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

    bool color_edit(const string& label, float* values, int32 components, float* reset_value, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);
    bool checkbox(const string& label, bool* value, bool reset_value, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);
    bool combo(const string& label, const char** list, int32 size, int32& current_idx, int32 reset_value, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);

    bool image_load(const string& label, int32 texture_name, const glm::vec2& size, bool& load_new, float column_width = ImGui::GetContentRegionAvail().x * 0.33f);
} // namespace mango

#endif // MANGO_IMGUI_HELPER_HPP
