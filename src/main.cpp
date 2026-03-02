#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
#include <optional>
#include <print>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <memory>
#include <vulkan/vulkan_raii.hpp>

class App {
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{nullptr, SDL_DestroyWindow};
	bool done{false};
	std::optional<vk::raii::Context> context{};
	std::optional<vk::raii::Instance> instance{};
	std::optional<vk::raii::SurfaceKHR> surface{};
	std::optional<vk::raii::PhysicalDevice> physicalDevice{};
	uint32_t graphicsQueueFamilyIndex{};
	std::optional<vk::raii::Device> device{};
	std::optional<vk::raii::Queue> graphicsQueue{};
	
  public:
	App() {
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Vulkan_LoadLibrary(nullptr);
		window.reset(SDL_CreateWindow("CNC Vulkan", 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE));
		auto vkGetInstanceProcAddr{reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr())};
		context.emplace(vkGetInstanceProcAddr);

		auto const vulkanVersion{context->enumerateInstanceVersion()};
		std::println("Vulkan {}.{}", VK_API_VERSION_MAJOR(vulkanVersion), VK_API_VERSION_MINOR(vulkanVersion));
	}

	~App() {
		// Since the destructor runs before RAII Cleanup, we can't call SDL_Quit() here.
	}

	void Init() {
		InitInstance();
		InitSurface();
		PickPhysicalDevice();
		InitDevice();
	}

	void Run() {
		while (!done) {
			for (SDL_Event event; SDL_PollEvent(&event);) {
				if (event.type == SDL_EVENT_QUIT)
					done = true;
			}
		}
	}

	private:
	void InitInstance() {
		vk::ApplicationInfo applicationinfo{};
		vk::InstanceCreateInfo instanceCreateInfo{};

		uint32_t extensionCount;
		instanceCreateInfo.ppEnabledExtensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
		instanceCreateInfo.enabledExtensionCount = extensionCount;

		instance.emplace(*context, instanceCreateInfo);
	}

	void InitSurface() {
		VkSurfaceKHR raw_surface;
		SDL_Vulkan_CreateSurface(window.get(), **instance, nullptr, &raw_surface);
		surface.emplace(*instance, raw_surface);
	}

	void PickPhysicalDevice(){
		auto physicalDevices{instance->enumeratePhysicalDevices()};
		physicalDevice.emplace(*instance, *physicalDevices.front());
		auto rawDeviceName {physicalDevice->getProperties().deviceName};
		std::string deviceName(rawDeviceName.data(), std::strlen(rawDeviceName));
		std::println("Device: {}", deviceName);

		graphicsQueueFamilyIndex = 0;
	}

	void InitDevice(){
		vk::DeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
		std::array queuePriorities{1.0f};
		queueCreateInfo.setQueuePriorities(queuePriorities);

		vk::DeviceCreateInfo deviceCreateInfo{};
		std::array queueCreateInfos{queueCreateInfo};
		deviceCreateInfo.setQueueCreateInfos(queueCreateInfos);
		std::array<const char * const, 1> enabledExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
		deviceCreateInfo.setPEnabledExtensionNames(enabledExtensions);
		
		device.emplace(*physicalDevice, deviceCreateInfo);

		graphicsQueue.emplace(*device, graphicsQueueFamilyIndex, 0);
	}
};

int main() {
	App app{};
	app.Init();
	app.Run();

	return 0;
}
