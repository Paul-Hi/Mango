project(editor)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor.cpp
)

add_executable(
    editor
        ${SOURCES}
)

set_target_properties(editor
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/bin
)

target_link_libraries(editor
    PUBLIC
        mango::mango
)
