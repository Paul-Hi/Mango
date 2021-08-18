project(mango)

set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/mango.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/application.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/context.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/log.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/assert.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/input_codes.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/profile.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/imgui_helper.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/mesh_factory.hpp

    # Display
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/display.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/display_event_handler.hpp
    # Input
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/input.hpp
    # Resources
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/resources.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/resource_structures.hpp
    # UI
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/ui.hpp
    # Scene
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/scene_structures.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/scene.hpp
    # Renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/renderer.hpp
    # Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/packed_freelist.hpp
)

set(PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/context_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/timer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/light_stack.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/render_data_builder.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/allocator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/linear_allocator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/free_list_allocator.hpp

    # Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/hashing.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/helpers.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/signal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/intersect.hpp
    # Display
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/display_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/display_event_handler_impl.hpp
    $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/glfw/glfw_display.hpp>
    $<$<BOOL:${LINUX}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/glfw/glfw_display.hpp>
    # Input
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/input_impl.hpp
    # Resources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/resources_impl.hpp
    # Scene
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene_internals.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene_impl.hpp
    # UI
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dear_imgui/imgui_opengl3.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dear_imgui/imgui_glfw.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dear_imgui/imgui_widgets.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dear_imgui/icons_font_awesome_5.hpp
    # Graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics_types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics_device.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics_device_context.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics_resources.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics_state.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics.hpp
    # OpenGL
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_graphics_device.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_graphics_device_context.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_graphics_resources.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_graphics_state.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_shader_program_cache.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_framebuffer_cache.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_vertex_array_cache.hpp
    # Renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/renderer_pipeline_cache.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/renderer_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/pipelines/deferred_pbr_renderer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/render_step.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/environment_display_step.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/shadow_map_step.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/fxaa_step.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/debug_drawer.hpp
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/context_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/environment_display_step.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/shadow_map_step.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/fxaa_step.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/mesh_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/light_stack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/render_data_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/linear_allocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/free_list_allocator.cpp


    # Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/intersect.cpp
    # Display
    $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/glfw/glfw_display.cpp>
    $<$<BOOL:${LINUX}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/glfw/glfw_display.cpp>
    # Input
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/input_impl.cpp
    # Graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics.cpp
    # Resources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/resources_impl.cpp
    # Scene
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene_impl.cpp
    # UI
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dear_imgui/imgui_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dear_imgui/imgui_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dear_imgui/imgui_helper.cpp
    # OpenGL
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_graphics_resources.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_graphics_device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_graphics_device_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_shader_program_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_framebuffer_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/opengl/gl_vertex_array_cache.cpp
    # Renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/renderer_pipeline_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/renderer_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/pipelines/deferred_pbr_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/debug_drawer.cpp
)

add_library(mango
    ${PUBLIC_HEADERS}
    ${PRIVATE_HEADERS}
    ${SOURCES}
)

add_library(mango::mango ALIAS mango)

target_include_directories(mango
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(mango SYSTEM
    PUBLIC
        ${CMAKE_SOURCE_DIR}/dependencies/glm
        ${CMAKE_SOURCE_DIR}/dependencies/glad/include
        ${CMAKE_SOURCE_DIR}/dependencies/optional/include
)

set_target_properties(mango
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/bin
)

target_link_libraries(mango
    PUBLIC
        spdlog::spdlog
        imgui
        tinyfd
        $<$<CONFIG:Release>:$<$<BOOL:${MANGO_PROFILE}>:tracy>>
        $<$<BOOL:${MANGO_BUILD_TESTS}>:glad>
        tiny_gltf
    PRIVATE
        ${OPENGL_LIBRARIES}
        glad
        glfw
        stb_image
)

target_compile_definitions(mango
    PUBLIC
        $<$<CONFIG:Debug>:MANGO_DEBUG>
        $<$<CXX_COMPILER_ID:MSVC>:MANGO_WINMAIN>
        $<$<CXX_COMPILER_ID:Clang>:$<$<BOOL:{CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"}>:MANGO_WINMAIN>>
        $<$<BOOL:${MANGO_BUILD_TESTS}>:MANGO_TEST>
        $<$<BOOL:${MANGO_PROFILE}>:MANGO_PROFILE>
        $<$<CXX_COMPILER_ID:MSVC>: _CRT_SECURE_NO_WARNINGS>
    PRIVATE
        $<$<BOOL:${WIN32}>:WIN32>
        $<$<BOOL:${LINUX}>:LINUX>
)

target_compile_options(mango
    PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:$<$<BOOL:${MANGO_ENABLE_HARD_WARNINGS}>: /WX>>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:$<$<BOOL:${MANGO_ENABLE_HARD_WARNINGS}>: -pedantic -Werror -Wconversion -pedantic-errors>>
)
