cmake_minimum_required(VERSION 3.30)

# As per https://github.com/Kitware/CMake/blob/v4.2.3/Help/dev/experimental.rst
# Needs a *version specific* UUID to be enabled (to be found in the link above with correct git tag)
# Also must come before the "project" call for whatever reason. ¯\_(ツ)_/¯
# Fingers crossed 4.4 fixes this mess https://gitlab.kitware.com/cmake/cmake/-/merge_requests/11745
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD d0edc3af-4c50-42ea-a356-e2862fe7a444)
set(CMAKE_CXX_MODULE_STD ON)

project(volumeVulkan)

# Project
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

# SDL family all need Shared vs Static to be in sync, so I set it here:
set(BUILD_SHARED_LIBS OFF)

# Enable the CPM Package Manager
file( DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.1/CPM.cmake ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake )
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# Packages

## Vulkan
CPMAddPackage("gh:KhronosGroup/Vulkan-Headers#v1.4.346")
CPMAddPackage("gh:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator#v3.3.0")

## Util
CPMAddPackage("gh:g-truc/glm#1.0.3")

## SDL Family
CPMAddPackage( URI "gh:libsdl-org/SDL#release-3.4.2"
	OPTIONS
		"SDL_RENDER OFF" # Remove SDL's renderer
		"SDL_GPU OFF" # Remove SDL's renderer
		"SDL_JOYSTICK OFF" # Disable Joystick subsystem
		"SDL_HAPTIC OFF" # Disale Joystick subsystem
)
CPMAddPackage("gh:libsdl-org/SDL_image#release-3.4.0")

add_executable(${PROJECT_NAME} src/SDL.cpp src/VMA.cpp)
file(GLOB_RECURSE Modules CONFIGURE_DEPENDS src/*.cppm)
target_sources(${PROJECT_NAME} PRIVATE FILE_SET CXX_MODULES FILES ${Modules})

target_compile_definitions(Vulkan-HppModule PRIVATE
	VULKAN_HPP_NO_STRUCT_CONSTRUCTORS # Enable designated initializers
)
target_compile_definitions(${PROJECT_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 SDL3_image::SDL3_image Vulkan-HppModule VulkanMemoryAllocator glm::glm)

## Assets Folder, for as long as I don't use #embed
add_custom_target(copy_assets COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets)
add_dependencies(${PROJECT_NAME} copy_assets)
