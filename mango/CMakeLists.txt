project(mango)

set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/mango.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/application.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/context.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/scene_types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/log.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/assert.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/window_system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/render_system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/scene.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/scene_component_manager.hpp
)

set(PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/context_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/window_system_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/render_system_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/pipelines/deferred_pbr_render_system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/pipeline_step.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/ibl_step.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/hashing.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/resource_system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/image_structures.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/model_structures.hpp

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
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/context_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/render_system_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/pipelines/deferred_pbr_render_system.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/steps/ibl_step.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/resource_system.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene.cpp

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
        ${CMAKE_SOURCE_DIR}/dependencies/glm
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
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
        $<$<BOOL:${MANGO_BUILD_TESTS}>:MANGO_TEST>
    PRIVATE
        $<$<BOOL:${WIN32}>:WIN32>
        $<$<BOOL:${LINUX}>:LINUX>
)
