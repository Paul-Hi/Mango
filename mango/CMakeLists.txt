project(mango)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/mango.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/application.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/context.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mango/types.hpp
)

set(PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/context_impl.hpp
)

set(SOURCES
${CMAKE_CURRENT_SOURCE_DIR}/src/core/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/context_impl.cpp
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

    PRIVATE
        ${OPENGL_LIBRARIES}
        glfw
        glad
)

target_compile_features(mango
    PUBLIC
        cxx_std_11
)

target_compile_definitions(mango
    PUBLIC
        $<$<CONFIG:Debug>:WIPE_DEBUG>
    PRIVATE
        $<$<BOOL:${WIN32}>:WIN32>
        $<$<BOOL:${LINUX}>:LINUX>
)
