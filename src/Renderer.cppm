export module renderer;

import vulkan;
import GPU;
import window;
import swapchain;

export class Renderer {
  public:
	Renderer(GPU &gpu, Window &window) : surface(gpu.createSurface(window)), swapchain(gpu.createSwapchain(surface, window.GetSize())) {
	}

  private:
	vk::raii::SurfaceKHR surface;
	Swapchain swapchain;
};
