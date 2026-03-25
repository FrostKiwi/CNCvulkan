export module renderer;

import std;
import vulkan;
import GPU;
import window;
import swapchain;

struct FrameResources {
	vk::raii::Semaphore imageAvailableSemaphore;
	vk::raii::Semaphore renderFinishedSemaphore;
	vk::raii::Fence inFlightFence;
};

export class Renderer {
  public:
	Renderer(GPU &gpu, Window &window) : surface(gpu.createSurface(window)), swapchain(gpu.createSwapchain(surface, window.GetSize())) {
		for (auto &frameResource : frameResources)
			frameResource.emplace(gpu.createSemaphore(), gpu.createSemaphore(), gpu.createFence());
	}

  private:
	static constexpr std::size_t IN_FLIGHT_FRAME_COUNT = 2;
	vk::raii::SurfaceKHR surface;
	Swapchain swapchain;
	std::array<std::optional<FrameResources>, IN_FLIGHT_FRAME_COUNT> frameResources;
};
