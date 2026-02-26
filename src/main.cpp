#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
#include <optional>
#include <print>
#include <vulkan/vulkan_core.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <memory>
#include <vulkan/vulkan_raii.hpp>

class App {
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{nullptr, SDL_DestroyWindow};
	bool done{false};
	std::optional<vk::raii::Context> context{};

  public:
	App() {
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Vulkan_LoadLibrary(nullptr);
		window.reset(SDL_CreateWindow("CNC Vulkan", 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE));
		auto vkGetInstanceProcAddr{reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr())};
		context.emplace(vkGetInstanceProcAddr);

		auto vulkanVersion{context->enumerateInstanceVersion()};
		std::println("Vulkan Version {}.{}", VK_API_VERSION_MAJOR(vulkanVersion), VK_API_VERSION_MINOR(vulkanVersion));
	}

	~App() {
		SDL_Quit();
	}

	void Init() {
	}

	void Run() {
		while (!done) {
			for (SDL_Event event; SDL_PollEvent(&event);) {
				if (event.type == SDL_EVENT_QUIT)
					done = true;
			}
		}
	}
};

int main() {
	App app{};
	app.Init();
	app.Run();

	return 0;
}
