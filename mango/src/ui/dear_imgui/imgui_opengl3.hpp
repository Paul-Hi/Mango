//! \file      imgui_opengl3.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef IMGUI_OPENGL3_HPP
#define IMGUI_OPENGL3_HPP

#include <imgui.h>

//! \cond NO_COND

IMGUI_IMPL_API bool ImGui_ImplOpenGL3_Init(const char* glsl_version = NULL);
IMGUI_IMPL_API void ImGui_ImplOpenGL3_Shutdown();
IMGUI_IMPL_API void ImGui_ImplOpenGL3_NewFrame();
IMGUI_IMPL_API void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);

IMGUI_IMPL_API bool ImGui_ImplOpenGL3_CreateFontsTexture();
IMGUI_IMPL_API void ImGui_ImplOpenGL3_DestroyFontsTexture();
IMGUI_IMPL_API bool ImGui_ImplOpenGL3_CreateDeviceObjects();
IMGUI_IMPL_API void ImGui_ImplOpenGL3_DestroyDeviceObjects();

// Defining this for now
#define IMGUI_IMPL_OPENGL_LOADER_GLAD

//! \endcond

#endif // IMGUI_OPENGL3_HPP