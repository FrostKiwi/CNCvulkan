module;
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <glm/vec2.hpp>

export module window;

import vulkan;
import std;

export class Window {
  public:
	auto CreateSurface(vk::raii::Instance &instance) -> vk::raii::SurfaceKHR {
		VkSurfaceKHR surface;
		if(!SDL_Vulkan_CreateSurface(window.get(), *instance, nullptr, &surface))
			throw std::runtime_error(SDL_GetError());

		return {instance, surface};
	};

	Window() : window(SDL_CreateWindow("CNC Vulkan", 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE), SDL_DestroyWindow) {
		if (!window)
			throw std::runtime_error(SDL_GetError());
	}

	auto GetInstanceRequiredExtensions() -> std::span<const char *const> {
		uint32_t count;
		return {SDL_Vulkan_GetInstanceExtensions(&count), count};
	}

	auto GetSize() -> glm::ivec2 {
		int width;
		int height;
		if (!SDL_GetWindowSizeInPixels(window.get(), &width, &height))
			throw std::runtime_error(SDL_GetError());

		return {width, height};
	}

  private:
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window;
};
