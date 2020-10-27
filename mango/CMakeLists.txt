project(mango)

set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/mango.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/application.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/context.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/scene_ecs.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/log.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/assert.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/window_system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/render_system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/scene.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/scene_component_pool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/input_system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/input_codes.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/ui_system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/profile.hpp
)

set(PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/context_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/window_system_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/render_system_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/input_system_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/pipelines/deferred_pbr_render_system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/pipeline_step.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/ibl_step.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/shadow_map_step.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/gpu_buffer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/hashing.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/helpers.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/signal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/resource_system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/image_structures.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/model_structures.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/timer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_system_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/ecs_internal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dear_imgui/imgui_opengl3.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dear_imgui/imgui_glfw.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dear_imgui/imgui_widgets.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/allocator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/linear_allocator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/free_list_allocator.hpp

    # graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics_common.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics_state.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/command_buffer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics_object.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/vertex_array.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/buffer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shader_program.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/texture.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/framebuffer.hpp
    # graphics impl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/impl/vertex_array_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/impl/buffer_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/impl/shader_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/impl/shader_program_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/impl/texture_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/impl/framebuffer_impl.hpp

    $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/win32_window_system.hpp>
    $<$<BOOL:${LINUX}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/linux_window_system.hpp>

    $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/win32_input_system.hpp>
    $<$<BOOL:${LINUX}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/linux_input_system.hpp>
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/context_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/render_system_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/pipelines/deferred_pbr_render_system.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/ibl_step.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/shadow_map_step.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/gpu_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/resource_system.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_system_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dear_imgui/imgui_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dear_imgui/imgui_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/linear_allocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/free_list_allocator.cpp

    # graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/command_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/vertex_array.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shader_program.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/framebuffer.cpp
    # graphics impl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/impl/vertex_array_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/impl/buffer_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/impl/shader_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/impl/shader_program_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/impl/texture_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/impl/framebuffer_impl.cpp

    $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/win32_window_system.cpp>
    $<$<BOOL:${LINUX}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/linux_window_system.cpp>

    $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/win32_input_system.cpp>
    $<$<BOOL:${LINUX}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/linux_input_system.cpp>
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
    PRIVATE
        ${OPENGL_LIBRARIES}
        glad
        glfw
        stb_image
        tiny_gltf
)

target_compile_definitions(mango
    PUBLIC
        $<$<CONFIG:Debug>:MANGO_DEBUG>
        $<$<CXX_COMPILER_ID:MSVC>:MANGO_WINMAIN>
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
