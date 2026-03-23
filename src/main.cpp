#include <SDL3/SDL_main.h>

import OldApp;
import std;

int Oldmain() {
	OldApp app {};
	app.Init();
	app.Run();

	return 0;
}

auto SDL_AppInit(void **appstate, int argc, char **argv) -> SDL_AppResult {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindow("CNC Vulkan", 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	return SDL_APP_CONTINUE;
}
auto SDL_AppIterate(void *appstate) -> SDL_AppResult {
	std::println("");
	return SDL_APP_CONTINUE;
}
auto SDL_AppEvent(void *appstate, SDL_Event *event) -> SDL_AppResult {
	switch (event->type) {
	case SDL_EVENT_QUIT:
		return SDL_APP_SUCCESS;
	default:
		return SDL_APP_CONTINUE;
	}
}
auto SDL_AppQuit(void *appstate, SDL_AppResult result) -> void {
}
