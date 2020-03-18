project(mango)

set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/mango.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/application.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/context.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/log.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/assert.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/system.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/window_system.hpp
)

set(PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/context_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/window_system_impl.hpp

    $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/win32_window_system.hpp>
    $<$<BOOL:${LINUX}>:${CMAKE_CURRENT_SOURCE_DIR}/src/core/linux_window_system.hpp>
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/context_impl.cpp

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
        glfw
        glad
)

target_compile_definitions(mango
    PUBLIC
        $<$<CONFIG:Debug>:MANGO_DEBUG>
    PRIVATE
        $<$<BOOL:${WIN32}>:WIN32>
        $<$<BOOL:${LINUX}>:LINUX>
)
