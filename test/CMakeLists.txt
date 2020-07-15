project(mangotests)

add_executable(AllTests
    mock_classes.hpp

    test_main.cpp
    init_test.cpp
    window_system_test.cpp
    render_system_test.cpp
)

target_include_directories(AllTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../mango/src
)

target_compile_definitions(AllTests
    PRIVATE
        $<$<BOOL:${WIN32}>:WIN32>
        $<$<BOOL:${LINUX}>:LINUX>
        $<$<CONFIG:Debug>:MANGO_DEBUG>
        MANGO_TEST
)

set_target_properties(AllTests
    PROPERTIES
		VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(AllTests
    gtest
    gmock
    gtest_main
    mango
)
