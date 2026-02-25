#include <SDL3/SDL.h>

struct {
	SDL_Window *window;
} App;

int main(){
	bool done = false;

	SDL_Init(SDL_INIT_VIDEO);

	App.window = SDL_CreateWindow("CNC Vulkan", 800, 600, 0);

	if (App.window == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create App.window: %s\n", SDL_GetError());
		return 1;		
	}

	while (!done) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				done = true;
			}
		}
	}

	SDL_DestroyWindow(App.window);

	SDL_Quit();
    return 0;
}
