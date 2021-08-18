//! \file      imgui_helper.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <mango/imgui_helper.hpp>
#include <ui/dear_imgui/icons_font_awesome_5.hpp>

using namespace mango;

void mango::column_split(const string& string_id, int32 number, float column_width)
{
    ImGui::BeginColumns(string_id.c_str(), number, ImGuiColumnsFlags_NoResize | ImGuiColumnsFlags_NoBorder);
    ImGui::SetColumnWidth(0, column_width);
}

void mango::column_next()
{
    ImGui::NextColumn();
}

void mango::column_merge()
{
    ImGui::EndColumns();
}

void mango::text_wrapped(const string& text)
{
    ImGui::BeginGroup();
    int64 last           = 0;
    int64 next           = 0;
    auto text_size       = ImGui::CalcTextSize(text.substr(last).c_str());
    auto available_width = ImGui::GetContentRegionAvail().x;
    while (text_size.x > available_width)
    {
        if ((next = text.find(" ", last)) != static_cast<int64>(string::npos))
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text(text.substr(last, next - last).c_str());
            last      = next + 1;
            text_size = ImGui::CalcTextSize(text.substr(last).c_str());
        }
        else
            break;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text(text.substr(last).c_str());
    ImGui::EndGroup();
}

void mango::custom_info(const string& label, std::function<void()> component_function, float group_width_modifier, float column_width)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGui::PushID(label.c_str());
    column_split("split", 2, column_width);

    text_wrapped(label);

    column_next();

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x + group_width_modifier);
    ImGui::BeginGroup();
    component_function();
    ImGui::EndGroup();
    ImGui::PopItemWidth();

    column_merge();
    ImGui::Spacing();
    ImGui::PopID();
}

bool mango::custom_aligned(const string& label, std::function<bool(bool reset)> component_function, float group_width_modifier, float column_width)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    bool value_changed = false;

    ImGui::PushID(label.c_str());
    column_split("split", 2, column_width);

    text_wrapped(label);

    bool reset = false;
    if (ImGui::IsItemClicked(1) && !ImGui::IsPopupOpen("##custom_element_options"))
    {
        ImGui::OpenPopup("##custom_element_options");
    }
    if (ImGui::BeginPopup("##custom_element_options"))
    {
        if (ImGui::Selectable(("Reset " + label).c_str()))
        {
            reset = true;
        }
        ImGui::EndPopup();
    }
    column_next();

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x + group_width_modifier);
    ImGui::BeginGroup();
    value_changed |= component_function(reset);
    ImGui::EndGroup();
    ImGui::PopItemWidth();

    column_merge();
    ImGui::Spacing();
    ImGui::PopID();

    return value_changed;
}

bool mango::drag_float_n(const string& label, float* values, int32 components, float* reset_value, float speed, float min_value, float max_value, const char* format, bool component_buttons,
                         float column_width)
{
    ImGuiContext& g   = *GImGui;
    float line_height = component_buttons ? g.Font->FontSize + g.Style.FramePadding.y * 2.0f : 0.0f;
    return custom_aligned(
        label,
        [line_height, &values, components, reset_value, speed, min_value, max_value, format, component_buttons](bool reset)
        {
            ImGuiContext& g    = *GImGui;
            bool value_changed = false;
            ImGui::PushMultiItemsWidths(components, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.5f, 0.0f, 0.0f, 1.0f });
            ImVec2 button_size = { line_height, line_height };

            const char* v[5] = { "X", "Y", "Z", "W", "" };

            bool at_least_one_reset = false;
            for (int32 i = 0; i < components; i++)
            {
                ImGui::PushID(i);
                if (i > 0)
                    ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);

                if (component_buttons)
                {
                    if (ImGui::Button(v[i > 4 ? 4 : i], button_size) || reset)
                    {
                        *values            = reset_value[i];
                        at_least_one_reset = true;
                    }
                    ImGui::SameLine(0, 0);
                }
                else if (reset)
                    *values = reset_value[i];

                value_changed |= ImGui::DragFloat("", values, speed, min_value, max_value, format);
                ImGui::PopID();
                ImGui::PopItemWidth();
                values = (float*)((char*)values + sizeof(float));
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            return value_changed | reset | at_least_one_reset;
        },
        -line_height * components, column_width);
}

bool mango::slider_float_n(const string& label, float* values, int32 components, float* reset_value, float min_value, float max_value, const char* format, bool component_buttons, float column_width)
{
    ImGuiContext& g   = *GImGui;
    float line_height = component_buttons ? g.Font->FontSize + g.Style.FramePadding.y * 2.0f : 0.0f;
    return custom_aligned(
        label,
        [line_height, &values, components, reset_value, min_value, max_value, format, component_buttons](bool reset)
        {
            ImGuiContext& g    = *GImGui;
            bool value_changed = false;
            ImGui::PushMultiItemsWidths(components, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.5f, 0.0f, 0.0f, 1.0f });
            ImVec2 button_size = { line_height, line_height };

            const char* v[5] = { "X", "Y", "Z", "W", "" };

            bool at_least_one_reset = false;
            for (int32 i = 0; i < components; i++)
            {
                ImGui::PushID(i);
                if (i > 0)
                    ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);

                if (component_buttons)
                {
                    if (ImGui::Button(v[i > 4 ? 4 : i], button_size) || reset)
                    {
                        *values            = reset_value[i];
                        at_least_one_reset = true;
                    }
                    ImGui::SameLine(0, 0);
                }
                else if (reset)
                    *values = reset_value[i];

                value_changed |= ImGui::SliderFloat("", values, min_value, max_value, format);
                ImGui::PopID();
                ImGui::PopItemWidth();
                values = (float*)((char*)values + sizeof(float));
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            return value_changed | reset | at_least_one_reset;
        },
        -line_height * components, column_width);
}

bool mango::slider_int_n(const string& label, int32* values, int32 components, int32* reset_value, int32 min_value, int32 max_value, const char* format, bool component_buttons, float column_width)
{
    ImGuiContext& g   = *GImGui;
    float line_height = component_buttons ? g.Font->FontSize + g.Style.FramePadding.y * 2.0f : 0.0f;
    return custom_aligned(
        label,
        [line_height, &values, components, reset_value, min_value, max_value, format, component_buttons](bool reset)
        {
            ImGuiContext& g    = *GImGui;
            bool value_changed = false;
            ImGui::PushMultiItemsWidths(components, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.5f, 0.0f, 0.0f, 1.0f });
            ImVec2 button_size = { line_height, line_height };

            const char* v[5] = { "X", "Y", "Z", "W", "" };

            bool at_least_one_reset = false;
            for (int32 i = 0; i < components; i++)
            {
                ImGui::PushID(i);
                if (i > 0)
                    ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);

                if (component_buttons)
                {
                    if (ImGui::Button(v[i > 4 ? 4 : i], button_size) || reset)
                    {
                        *values            = reset_value[i];
                        at_least_one_reset = true;
                    }
                    ImGui::SameLine(0, 0);
                }
                else if (reset)
                    *values = reset_value[i];

                value_changed |= ImGui::SliderInt("", values, min_value, max_value, format);
                ImGui::PopID();
                ImGui::PopItemWidth();
                values = (int32*)((char*)values + sizeof(int32));
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            return value_changed | reset | at_least_one_reset;
        },
        -line_height * components, column_width);
}

bool mango::color_edit(const string& label, float* values, int32 components, float* reset_value, float column_width)
{
    return custom_aligned(
        label,
        [&values, components, reset_value](bool reset)
        {
            bool value_changed = false;
            ImGui::PushItemWidth(ImGui::CalcItemWidth());
            if (components == 4)
                value_changed = ImGui::ColorEdit4("##edit4", values, ImGuiColorEditFlags_NoInputs);
            else
                value_changed = ImGui::ColorEdit3("##edit3", values, ImGuiColorEditFlags_NoInputs);
            ImGui::PopItemWidth();
            if (reset)
            {
                for (int32 i = 0; i < components; i++)
                {
                    *values = reset_value[i];
                    values  = (float*)((char*)values + sizeof(float));
                }
            }
            return value_changed | reset;
        },
        0.0f, column_width);
}

bool mango::checkbox(const string& label, bool* value, bool reset_value, float column_width)
{
    return custom_aligned(
        label,
        [&value, reset_value](bool reset)
        {
            bool value_changed = false;
            if (reset)
                *value = reset_value;
            ImGui::PushItemWidth(ImGui::CalcItemWidth());
            value_changed = ImGui::Checkbox("##check", value);
            ImGui::PopItemWidth();
            return value_changed | reset;
        },
        0.0f, column_width);
}

bool mango::combo(const string& label, const char** list, int32 size, int32& current_idx, int32 reset_value, float column_width)
{
    return custom_aligned(
        label,
        [list, size, &current_idx, reset_value](bool reset)
        {
            bool value_changed = false;
            ImGui::PushItemWidth(ImGui::CalcItemWidth());
            if (ImGui::BeginCombo("", list[current_idx]))
            {
                for (int32 n = 0; n < size; ++n)
                {
                    bool is_selected = (n == current_idx);
                    auto tp          = list[n];
                    if (ImGui::Selectable(tp, is_selected))
                    {
                        current_idx   = n;
                        value_changed = true;
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            if (reset)
                current_idx = reset_value;
            return value_changed | reset;
        },
        0.0f, column_width);
}

bool mango::image_load(const string& label, void* texture_native_handle, const vec2& size, bool& load_new, float column_width)
{
    return custom_aligned(
        label,
        [&texture_native_handle, &size, &load_new](bool reset)
        {
            load_new = false;
            ImVec2 canvas_p0;
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
            canvas_p0 = ImGui::GetCursorScreenPos();
            draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + size.x, canvas_p0.y + size.y), IM_COL32(127, 127, 127, 255), 2.0f);
            if (texture_native_handle)
                ImGui::Image(texture_native_handle, ImVec2(size.x, size.y));
            else
                ImGui::Dummy(ImVec2(size.x, size.y));
            if (ImGui::IsItemHovered())
            {
                if (texture_native_handle)
                {
                    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 0.5, 1.0));
                    ImGui::BeginTooltip();
                    ImGui::Image(texture_native_handle, ImVec2(size.x * 4.0f, size.y * 4.0f));
                    ImGui::EndTooltip();
                    ImGui::PopStyleColor();
                }
                else
                    ImGui::SetTooltip("Load");
                draw_list->AddRect(canvas_p0, ImVec2(canvas_p0.x + size.x, canvas_p0.y + size.y), IM_COL32(200, 200, 200, 255), 2.0f);
            }
            ImGui::PopStyleVar();
            if (ImGui::IsItemClicked())
            {
                load_new = true;
                return true;
            }
            if (reset || ImGui::IsItemClicked(1))
                return true;
            return false;
        },
        0.0f, column_width);
}
