#include <SDL3/SDL.h>

int main(){
	SDL_Window *window;
	bool done = false;

	SDL_Init(SDL_INIT_VIDEO);

	SDL_CreateWindow("CNC Vulkan", 800, 600, 0);

	if (window == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
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

	SDL_DestroyWindow(window);

	SDL_Quit();
    return 0;
}
