project(editor)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor.cpp
)

if(WIN32 AND MSVC)
    add_executable(
        editor WIN32
            ${SOURCES}
    )
else()
        add_executable(
        editor
            ${SOURCES}
    )
endif()

set_target_properties(editor
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/bin
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(editor
    PUBLIC
        mango::mango
)

target_compile_definitions(editor
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>: _CRT_SECURE_NO_WARNINGS>
    )

target_compile_options(editor
    PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:$<$<BOOL:${MANGO_ENABLE_HARD_WARNINGS}>: /WX>>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:$<$<BOOL:${MANGO_ENABLE_HARD_WARNINGS}>: -pedantic -Werror -Wconversion -pedantic-errors>>
)

install(TARGETS editor DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/res/ DESTINATION bin/res)
install(FILES ${CMAKE_SOURCE_DIR}/imgui.ini DESTINATION bin)
