cmake_minimum_required(VERSION 3.10)

project(MangoGraphicsEngine
    VERSION 1.0
    DESCRIPTION "The Mango Engine."
    LANGUAGES CXX
)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()


set(CMAKE_COLOR_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug, Release" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install/mango/" CACHE PATH "Install path prefix" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

option(MANGO_BUILD_DOC   "Build documentation" ON)
option(MANGO_PROFILE "Enables profiling when build is Release!" OFF)
option(MANGO_BUILD_TESTS "Build Unit Tests" OFF)
option(MANGO_ENABLE_HARD_WARNINGS "Enables some compiler parameters. This should not be enabled, Mango will NOT build." OFF)

set(VERSION_MAJOR 0 CACHE STRING "Project major version number.")
set(VERSION_MINOR 0 CACHE STRING "Project minor version number.")
set(VERSION_PATCH 1 CACHE STRING "Project patch version number.")
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH)
message(STATUS "Mango Build type: " ${CMAKE_BUILD_TYPE})

# Add path to glm files for find_package.
set(CMAKE_PREFIX_PATH "dependencies/glm/cmake/glm/")
macro(find_package_verbose)
        find_package(${ARGV0})
        message(STATUS "Found ${ARGV0}.")
endmacro()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    message(STATUS "Setting project standards.")
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    if(MANGO_BUILD_DOC)
        find_package_verbose(Doxygen REQUIRED dot)
        if(DOXYGEN_FOUND)
            add_subdirectory(doc)
        else()
            message(WARNING "Doxygen not found, documentation will not be build. Please Install Doxygen to build the documentation!")
        endif()
    endif()
endif()

file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file).
                Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")

set(OpenGL_GL_PREFERENCE "GLVND")
find_package_verbose(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
add_subdirectory(dependencies/glfw)
message(STATUS "Added glfw.")

add_subdirectory(dependencies/glad)
message(STATUS "Added GLAD.")

add_subdirectory(dependencies/spdlog)
message(STATUS "Added spdlog.")

add_subdirectory(dependencies/stb_image)
message(STATUS "Added stb_image.")

add_subdirectory(dependencies/tiny_gltf)
message(STATUS "Added tiny_gltf.")

add_subdirectory(dependencies/imgui)
message(STATUS "Added dear imgui.")

add_subdirectory(dependencies/tinyfd)
message(STATUS "Added tiny file dialogs.")

if(MANGO_PROFILE)
    add_subdirectory(dependencies/tracy)
    message(STATUS "Added tracy.")
endif()

find_package_verbose(glm REQUIRED)

if(MANGO_BUILD_TESTS)
    if(WIN32)
        set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib.")
    endif()
    add_subdirectory(dependencies/googletest)
    message(STATUS "Added googletest.")
    add_subdirectory(test)
endif()

add_subdirectory(mango)
#spdlog_enable_warnings(mango)
add_subdirectory(editor)
