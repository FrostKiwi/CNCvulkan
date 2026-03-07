#include <print>

#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
#include <vulkan/vulkan_raii.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

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

	std::optional<vk::raii::CommandPool> commandPool{};
	std::vector<vk::raii::CommandBuffer> commandBuffers{};
	std::vector<vk::raii::Fence> fences{};
	
	std::optional<vk::raii::SwapchainKHR> swapchain{};
	std::vector<vk::Image> swapchainImages{};
	vk::Extent2D swapchainExtent{};
	vk::Format swapchainImageFormat{vk::Format::eB8G8R8A8Srgb};

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
		InitCommandPool();
		AllocateCommandBuffers();
		InitSyncObjects();
		RecreateSwapchain();
	}

	void Run() {
		while (!done) {
			HandleEvents();
			Render();
		}
	}

	private:
	void Render(){
		device->resetFences(*fences[0]);
	}

	void HandleEvents(){
		for (SDL_Event event; SDL_PollEvent(&event);) {
			switch (event.type) {
				case SDL_EVENT_QUIT:
					done = true;
					break;

				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:					
					RecreateSwapchain();
					break;

				default:
					break;
			}
		}
	}
	void RecreateSwapchain() {
		vk::SurfaceCapabilitiesKHR surfaceCapabilities{physicalDevice->getSurfaceCapabilitiesKHR(*surface)};
		swapchainExtent = surfaceCapabilities.currentExtent;

		vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
		swapchainCreateInfo.surface = *surface;
		swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
		swapchainCreateInfo.imageFormat = swapchainImageFormat;
		swapchainCreateInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear; // Should be default anyways?
		swapchainCreateInfo.imageExtent = swapchainExtent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		swapchainCreateInfo.presentMode = vk::PresentModeKHR::eMailbox;
		swapchainCreateInfo.clipped = true;

		// Figure out why old swapchain handle is invalid
		//if (swapchain.has_value())
		//	swapchainCreateInfo.oldSwapchain = **swapchain;

		swapchain.emplace(*device, swapchainCreateInfo);
		swapchainImages = swapchain->getImages();
	}

	void InitSyncObjects() {
		vk::FenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

		fences.emplace_back(*device, fenceCreateInfo);
	}

	void AllocateCommandBuffers() {
		vk::CommandBufferAllocateInfo allocateInfo{};
		allocateInfo.commandPool = *commandPool;
		allocateInfo.commandBufferCount = 1;

		commandBuffers = device->allocateCommandBuffers(allocateInfo);
	}

	void InitCommandPool(){
		vk::CommandPoolCreateInfo commandPoolCreateInfo{};
		commandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
		commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		commandPool.emplace(*device, commandPoolCreateInfo);
	}

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
