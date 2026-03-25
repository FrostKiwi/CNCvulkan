import std;
import App;

#include <SDL3/SDL_main.h>

auto SDL_AppInit(void **appstate, int argc, char **argv) -> SDL_AppResult {
	auto app = std::make_unique<App>();
	const auto result = app->Init();
	*appstate = app.release();
	return result;
}

auto SDL_AppIterate(void *appstate) -> SDL_AppResult {
	auto app = static_cast<App *>(appstate);
	return app->Iterate();
}

auto SDL_AppEvent(void *appstate, SDL_Event *event) -> SDL_AppResult {
	auto app = static_cast<App *>(appstate);
	return app->Event(event);
}

auto SDL_AppQuit(void *appstate, SDL_AppResult result) -> void {
	std::unique_ptr<App> app {static_cast<App *>(appstate)};
	app->Quit(result);
}
