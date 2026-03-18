cmake_minimum_required(VERSION 3.30)

# As per https://github.com/Kitware/CMake/blob/v4.2.3/Help/dev/experimental.rst
# Needs a *version specific* UUID to be enabled (to be found in the link above with correct git tag)
# Also must come before the "project" call for whatever reason. ¯\_(ツ)_/¯
# Fingers crossed 4.4 fixes this mess https://gitlab.kitware.com/cmake/cmake/-/merge_requests/11745
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD d0edc3af-4c50-42ea-a356-e2862fe7a444)

project(volumeVulkan)

# Project
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(BUILD_SHARED_LIBS OFF) # Build SDL3 Statically (and any other that consume this define)

# Enable the CPM Package Manager
file( DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.1/CPM.cmake ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake )
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# Packages
CPMAddPackage("gh:KhronosGroup/Vulkan-Headers#v1.4.344")
CPMAddPackage("gh:g-truc/glm#1.0.3")
CPMAddPackage("gh:libsdl-org/SDL#release-3.4.2")

add_executable(${PROJECT_NAME} src/main.cpp)
file(GLOB_RECURSE Modules CONFIGURE_DEPENDS src/*.cppm)
target_sources(${PROJECT_NAME} PRIVATE FILE_SET CXX_MODULES FILES ${Modules})

# VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0: Disable HPP Dynamic Loader, because SDL3 already does it
# VULKAN_HPP_NO_STRUCT_CONSTRUCTORS: Enalbe designated initializers
target_compile_definitions(Vulkan-HppModule PRIVATE VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 Vulkan-HppModule glm::glm)
