module;
#include <SDL3/SDL_video.h>

export module window;

import vulkan;
import std;

export class Window {
  public:
	vk::raii::SurfaceKHR CreateSurface(vk::raii::Instance &instance);

	Window() : window(SDL_CreateWindow("CNC Vulkan", 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE), SDL_DestroyWindow) {
		if(!window)
			throw std::runtime_error(SDL_GetError());
	}

  private:
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window;
};
