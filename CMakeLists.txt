cmake_minimum_required(VERSION 3.30)
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
target_compile_definitions(${PROJECT_NAME} PRIVATE VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 Vulkan-Headers glm::glm)
