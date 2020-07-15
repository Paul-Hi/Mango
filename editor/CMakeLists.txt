project(editor)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor.cpp
)

if(WIN32)
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

target_compile_options(editor
    PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror -Wconversion -pedantic-errors>
)
