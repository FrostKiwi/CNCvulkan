cmake_minimum_required(VERSION 3.24)
project(volumeVulkan)

# Project
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CPM Repo
file(
	DOWNLOAD
	https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.1/CPM.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
	EXPECTED_HASH SHA256=f3a6dcc6a04ce9e7f51a127307fa4f699fb2bade357a8eb4c5b45df76e1dc6a5
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# Packages
CPMAddPackage("gh:KhronosGroup/Vulkan-Headers#v1.4.344")
CPMAddPackage("gh:libsdl-org/SDL#release-3.4.2")

# Tell Ninja to tell Clang to color the output
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	add_compile_options (-fcolor-diagnostics -fansi-escape-codes)
endif ()

add_executable(${PROJECT_NAME} src/main.cpp)

# Disable HPP Dynamic Loader, because SDL3 already does it
target_compile_definitions(${PROJECT_NAME} PRIVATE VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 Vulkan-Headers)
target_link_options(${PROJECT_NAME} PRIVATE)
