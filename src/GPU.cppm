export module GPU;

import vulkan;
import window;
import std;

export class GPU {

  public:
	GPU(Window &window) : instance(makeInstance(context, window.GetInstanceRequiredExtensions())) {
		auto physical_device_and_queue_family = selectPhysicalDevice(instance);

		if (!physical_device_and_queue_family)
			throw std::runtime_error("No suitable physical device");

		physicalDevice.emplace(physical_device_and_queue_family->first);
		queueFamilyIndex = physical_device_and_queue_family->second;

		std::array requiredExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		device.emplace()
	}

  private:
	vk::raii::Context context;
	vk::raii::Instance instance;

	std::optional<vk::raii::PhysicalDevice> physicalDevice;
	std::uint32_t queueFamilyIndex;
	std::optional<vk::raii::Device> device;
	std::optional<vk::raii::Queue> queue;

	auto makeInstance(vk::raii::Context &context, std::span<const char *const> requiredExtensions) -> vk::raii::Instance {
		vk::ApplicationInfo applicationinfo {.apiVersion = vk::ApiVersion14};
		vk::InstanceCreateInfo instanceCreateInfo {.pApplicationInfo = &applicationinfo};
		instanceCreateInfo.setPEnabledExtensionNames(requiredExtensions);

		vk::raii::Instance instance {context, instanceCreateInfo};
		return instance;
	}

	auto selectPhysicalDevice(vk::raii::Instance &instance) -> std::optional<std::pair<vk::raii::PhysicalDevice, std::uint32_t>> {
		auto physicalDevices {instance.enumeratePhysicalDevices()};

		for (auto physicalDevice : physicalDevices) {
			std::println("Device: {}", physicalDevice.getProperties().deviceName.data());
		}

		// Hard code graphics queue Family for now
		[[deprecated("Hard coded PhysicalDevice choice and Family")]]
		std::uint32_t graphicsQueueFamilyIndex = 0;

		// Optional unused for now, just return first device for now
		return {{physicalDevices.front(), graphicsQueueFamilyIndex}};
	}
};
