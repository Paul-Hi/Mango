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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/slotmap.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/intersect.hpp
)

set(PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/context_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/timer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/allocator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/linear_allocator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/free_list_allocator.hpp

    # Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/hashing.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/helpers.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/signal.hpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene_structures_internal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene_helper.hpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/renderer_bindings.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/renderer_pipeline_cache.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/renderer_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/pipelines/deferred_pbr_renderer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/render_pass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/environment_display_pass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/shadow_map_pass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/fxaa_pass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/light_stack.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/render_data_builder.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/debug_drawer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/render_pass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/deferred_lighting_pass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/geometry_pass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/transparent_pass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/composing_pass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/auto_luminance_pass.hpp

    #generated
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/shader_interop.hpp
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/context_impl.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/mesh_factory.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene_helper.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/light_stack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/render_data_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/environment_display_pass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/shadow_map_pass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/fxaa_pass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/deferred_lighting_pass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/geometry_pass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/transparent_pass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/composing_pass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/passes/auto_luminance_pass.cpp
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
        ${CMAKE_CURRENT_SOURCE_DIR}/gen
)

target_include_directories(mango SYSTEM
    PUBLIC
        ${CMAKE_SOURCE_DIR}/dependencies/eigen
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
        # Eigen3::Eigen
        spdlog::spdlog
        imgui
        tinyfd
        $<$<CONFIG:Release>:$<$<BOOL:${MANGO_PROFILE}>:tracy>>
        $<$<BOOL:${MANGO_BUILD_TESTS}>:glad>
        tinygltf
    PRIVATE
        ${OPENGL_LIBRARIES}
        glad
        glfw
        stb
)

target_compile_definitions(mango
    PUBLIC
        $<$<CONFIG:Debug>:MANGO_DEBUG>
        $<$<CXX_COMPILER_ID:MSVC>:MANGO_WINMAIN>
        $<$<CXX_COMPILER_ID:Clang>:$<$<BOOL:{ CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC" }>:MANGO_WINMAIN>>
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
