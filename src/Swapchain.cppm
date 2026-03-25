module;
#include <glm/vec2.hpp>

export module swapchain;

import vulkan;

export class Swapchain {
  public:
	Swapchain(vk::raii::PhysicalDevice &physicalDevice, vk::raii::Device &device, vk::raii::SurfaceKHR &surface, glm::ivec2 size) {
		auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

		swapchain.emplace(
			device,
			vk::SwapchainCreateInfoKHR {
				.surface = surface,
				.minImageCount = surfaceCapabilities.minImageCount + 1,
				.imageFormat = vk::Format::eR8G8B8A8Srgb,
				.imageExtent = vk::Extent2D {
					.width = static_cast<uint32_t>(size.x),
					.height = static_cast<uint32_t>(size.y)
				},
				.imageArrayLayers = 1,
				.imageUsage = vk::ImageUsageFlagBits::eTransferDst,
				.clipped = true
			});

		images = swapchain->getImages();
	};

  private:
	std::optional<vk::raii::SwapchainKHR> swapchain;
	std::vector<vk::Image> images;
};
