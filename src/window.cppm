module;
#include <SDL3/SDL.h>

export module window;

import vulkan;
import std;

export class Window {
  public:
  private:
	  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window;
};
