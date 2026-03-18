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

# CPM Repo
file(
	DOWNLOAD
	https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.1/CPM.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# Packages
CPMAddPackage("gh:KhronosGroup/Vulkan-Headers#v1.4.344")
CPMAddPackage("gh:g-truc/glm#1.0.3")
CPMAddPackage(
	URI "gh:libsdl-org/SDL#release-3.4.2"
	OPTIONS
		"BUILD_SHARED_LIBS OFF" # Build Static
)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)
target_sources(${PROJECT_NAME} PRIVATE FILE_SET CXX_MODULES FILES src/App.cppm src/util.cppm)
# Disable HPP Dynamic Loader, because SDL3 already does it
target_compile_definitions(Vulkan-HppModule PRIVATE VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 Vulkan-HppModule glm::glm)
