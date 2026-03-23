export module GPU;

import vulkan;
import std;

export class GPU {

  private:
	vk::raii::Context context;
	vk::raii::Instance instance;

	std::optional<vk::raii::PhysicalDevice> physicalDevice;
	std::uint32_t queueFamilyIndex;
	std::optional<vk::raii::Device> device;
	std::optional<vk::raii::Queue> queue;

	auto makeInstance(vk::raii::Context &context, std::span<const char *const> requiredExtensions) -> vk::raii::Instance {
	}
};
