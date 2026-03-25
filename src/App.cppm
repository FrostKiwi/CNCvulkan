module;
#include <SDL3/SDL_init.h>

export module App;
import std;
import window;
import GPU;
import renderer;

export class App {
  public:
	App() {
	}

	auto Init() -> SDL_AppResult {
		window.emplace();
		gpu.emplace(*window);
		renderer.emplace(*gpu, *window);
		return SDL_APP_CONTINUE;
	}

	auto Event(SDL_Event *event) -> SDL_AppResult {
		switch (event->type) {
		case SDL_EVENT_QUIT:
			return SDL_APP_SUCCESS;
		default:
			return SDL_APP_CONTINUE;
		}
	}

	auto Iterate() -> SDL_AppResult {
		return SDL_APP_CONTINUE;
	}

	auto Quit(SDL_AppResult result) -> void {
	}

  private:
	std::optional<Window> window;
	std::optional<GPU> gpu;
	std::optional<Renderer> renderer;
};
