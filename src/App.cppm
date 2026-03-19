module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <glm/glm.hpp>

export module App;

import vulkan;

import util;

struct Frame {
	vk::raii::CommandBuffer commandBuffer;
	vk::raii::Semaphore imageAvailableSemaphore;
	vk::raii::Semaphore renderFinishedSemaphore;
	vk::raii::Fence fence;
};

// 1 would be "CPU waits for GPU", n would be "CPU is n-1 Frames ahead"
constexpr uint32_t IN_FLIGHT_FRAME_COUNT {2};

export class App {
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window {nullptr, SDL_DestroyWindow};
	bool done {false};
	std::optional<vk::raii::Context> context {};
	std::optional<vk::raii::Instance> instance {};
	std::optional<vk::raii::SurfaceKHR> surface {};
	std::optional<vk::raii::PhysicalDevice> physicalDevice {};
	uint32_t graphicsQueueFamilyIndex {};
	std::optional<vk::raii::Device> device {};
	std::optional<vk::raii::Queue> graphicsQueue {};

	std::optional<vk::raii::CommandPool> commandPool {};
	std::array<std::optional<Frame>, IN_FLIGHT_FRAME_COUNT> frames {};
	uint32_t frameIndex {0};

	std::optional<vk::raii::SwapchainKHR> swapchain {};
	std::vector<vk::Image> swapchainImages {};
	vk::Extent2D swapchainExtent {};
	vk::Format swapchainImageFormat {vk::Format::eB8G8R8A8Srgb};
	uint32_t currentSwapchainImageIndex {};

  public:
	App() {
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Vulkan_LoadLibrary(nullptr);
		window.reset(SDL_CreateWindow("CNC Vulkan", 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE));
		auto vkGetInstanceProcAddr {reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr())};
		context.emplace(vkGetInstanceProcAddr);

		auto const vulkanVersion {context->enumerateInstanceVersion()};
		std::println("Vulkan {}.{}", vk::apiVersionMajor(vulkanVersion), vk::apiVersionMinor(vulkanVersion));
	}

	~App() {
		// Since the destructor runs before RAII Cleanup, we need to wait before calling SDL_Quit() here.
		device->waitIdle();
		SDL_Quit();
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
	void Render() {
		auto const frame & {*frames[frameIndex]};
		device->waitForFences(frame.fence, true, UINT64_MAX);
		device->resetFences(fence);

		// We are violating Vulkan spec here, see https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
		auto [acquireResult, imageIndex] = swapchain->acquireNextImage(UINT64_MAX, imageAvailableSemaphores[0], nullptr);
		currentSwapchainImageIndex = imageIndex;

		auto const &swapchainImage {swapchainImages[imageIndex]};

		vk::raii::CommandBuffer &commandBuffer {commandBuffers[0]};
		commandBuffer.reset();
		const vk::CommandBufferBeginInfo beginInfo {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
		commandBuffer.begin(beginInfo);

		double time = SDL_GetTicks() * 0.001;
		const glm::vec4 rainbow = hsv_to_rgb(glm::vec4 {sin(time) * 0.5 + 0.5, 1.0f, 1.0f, 1.0f});
		const vk::ClearColorValue color {rainbow.r, rainbow.g, rainbow.b, rainbow.a};

		// transfer image layout to transfer destination
		vk::ImageMemoryBarrier const barrier {
			.srcAccessMask = vk::AccessFlagBits::eMemoryRead,
			.dstAccessMask = vk::AccessFlagBits::eTransferWrite,
			.oldLayout = vk::ImageLayout::eUndefined,
			.newLayout = vk::ImageLayout::eTransferDstOptimal,
			.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
			.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
			.image = swapchainImage,
			.subresourceRange = vk::ImageSubresourceRange {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags {}, nullptr, nullptr, barrier);

		commandBuffer.clearColorImage(swapchainImage, vk::ImageLayout::eTransferDstOptimal, color, vk::ImageSubresourceRange {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

		vk::ImageMemoryBarrier const barrier2 {
			.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
			.dstAccessMask = vk::AccessFlagBits::eMemoryRead,
			.oldLayout = vk::ImageLayout::eTransferDstOptimal,
			.newLayout = vk::ImageLayout::ePresentSrcKHR,
			.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
			.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
			.image = swapchainImage,
			.subresourceRange = vk::ImageSubresourceRange {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags {}, nullptr, nullptr, barrier2);
		commandBuffer.end();

		vk::SubmitInfo submitInfo {};
		submitInfo.setCommandBuffers(*commandBuffer);
		submitInfo.setWaitSemaphores(*imageAvailableSemaphores[0]);
		submitInfo.setSignalSemaphores(*renderFinishedSemaphores[0]);
		constexpr vk::PipelineStageFlags waitStage {vk::PipelineStageFlagBits::eTransfer};
		submitInfo.setWaitDstStageMask(waitStage);
		graphicsQueue->submit(submitInfo, fence);

		vk::PresentInfoKHR presentInfo {};
		presentInfo.setSwapchains(**swapchain);
		presentInfo.setImageIndices(imageIndex);
		presentInfo.setWaitSemaphores(*renderFinishedSemaphores[0]);
		graphicsQueue->presentKHR(presentInfo);

		frameIndex = (frameIndex + 1) % IN_FLIGHT_FRAME_COUNT;
	}

	void HandleEvents() {
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
		vk::SurfaceCapabilitiesKHR surfaceCapabilities {physicalDevice->getSurfaceCapabilitiesKHR(*surface)};
		swapchainExtent = surfaceCapabilities.currentExtent;

		vk::SwapchainCreateInfoKHR swapchainCreateInfo {
			.surface = *surface,
			.minImageCount = surfaceCapabilities.minImageCount + 1,
			.imageFormat = swapchainImageFormat,
			.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear, // Should be default anyways?
			.imageExtent = swapchainExtent,
			.imageArrayLayers = 1,
			.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
			.preTransform = surfaceCapabilities.currentTransform,
			.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode = vk::PresentModeKHR::eMailbox,
			.clipped = true,
		};

		// Figure out why old swapchain handle is invalid
		// if (swapchain.has_value())
		//	swapchainCreateInfo.oldSwapchain = **swapchain;

		swapchain.emplace(*device, swapchainCreateInfo);
		swapchainImages = swapchain->getImages();
	}

	void InitSyncObjects() {
		vk::FenceCreateInfo fenceCreateInfo {};
		fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

		fences.emplace_back(*device, fenceCreateInfo);

		vk::SemaphoreCreateInfo semaphoreCreateInfo {};
		imageAvailableSemaphores.emplace_back(*device, semaphoreCreateInfo);
		renderFinishedSemaphores.emplace_back(*device, semaphoreCreateInfo);
	}

	void AllocateCommandBuffers() {
		vk::CommandBufferAllocateInfo allocateInfo {};
		allocateInfo.commandPool = *commandPool;
		allocateInfo.commandBufferCount = 1;

		commandBuffers = device->allocateCommandBuffers(allocateInfo);
	}

	void InitCommandPool() {
		vk::CommandPoolCreateInfo commandPoolCreateInfo {};
		commandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
		commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		commandPool.emplace(*device, commandPoolCreateInfo);
	}

	void InitInstance() {
		vk::ApplicationInfo applicationinfo {};
		vk::InstanceCreateInfo instanceCreateInfo {};

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

	void PickPhysicalDevice() {
		auto physicalDevices {instance->enumeratePhysicalDevices()};
		physicalDevice.emplace(*instance, *physicalDevices.front());
		auto rawDeviceName {physicalDevice->getProperties().deviceName};
		std::string deviceName(rawDeviceName.data(), std::strlen(rawDeviceName));
		std::println("Device: {}", deviceName);

		graphicsQueueFamilyIndex = 0;
	}

	void InitDevice() {
		vk::DeviceQueueCreateInfo queueCreateInfo {};
		queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
		std::array queuePriorities {1.0f};
		queueCreateInfo.setQueuePriorities(queuePriorities);

		vk::DeviceCreateInfo deviceCreateInfo {};
		std::array queueCreateInfos {queueCreateInfo};
		deviceCreateInfo.setQueueCreateInfos(queueCreateInfos);
		std::array<const char *const, 1> enabledExtensions {vk::KHRSwapchainExtensionName};
		deviceCreateInfo.setPEnabledExtensionNames(enabledExtensions);

		device.emplace(*physicalDevice, deviceCreateInfo);

		graphicsQueue.emplace(*device, graphicsQueueFamilyIndex, 0);
	}
};
