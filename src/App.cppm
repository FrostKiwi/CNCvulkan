module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3_image/SDL_image.h>
#include <glm/glm.hpp>

// Stick with simple VMA, but switch to VMA HPP RAII, once it's time
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"

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
	std::unique_ptr<VmaAllocator_T, decltype(&vmaDestroyAllocator)> allocator {nullptr, vmaDestroyAllocator};

	std::optional<vk::raii::CommandPool> commandPool {};
	std::array<std::optional<Frame>, IN_FLIGHT_FRAME_COUNT> frames {};
	uint32_t frameIndex {};

	std::optional<vk::raii::SwapchainKHR> swapchain {};
	std::vector<vk::Image> swapchainImages {};
	vk::Extent2D swapchainExtent {};
	vk::Format swapchainImageFormat {vk::Format::eB8G8R8A8Srgb};
	uint32_t currentSwapchainImageIndex {};

	std::optional<vk::raii::Image> texture;
	uint32_t textureWidth {};
	uint32_t textureHeight {};

	static constexpr auto vulkanVersion {vk::makeApiVersion(0, 1, 4, 0)};

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
		// SDL Pulls out the context from underneath vk cleanup, so this all segfaults here on exit
		// This doesn't work with RAII, gotta move it out of the class
		SDL_Quit();
	}

	void Init() {
		InitInstance();
		InitSurface();
		PickPhysicalDevice();
		InitDevice();
		InitVMAAllocator();
		InitCommandPool();
		InitFrames();
		RecreateSwapchain();

		// Make this load via #embed instead later on
		auto const image {IMG_Load("assets/matcap/basic_side_diffuse.png")};
		if (!image)
			throw;

		if (image->format != SDL_PIXELFORMAT_ABGR8888)
			throw;

		textureWidth = image->w;
		textureHeight = image->h;

		// Staging buffer, naivly assuming 8bpp 4 Channels
		vk::BufferCreateInfo bufferCreateInfo {
			.size = (vk::DeviceSize)(image->w * image->h * 4),
			.usage = vk::BufferUsageFlagBits::eTransferSrc
		};
		VmaAllocationCreateInfo allocationCreateInfo {
			.usage = VMA_MEMORY_USAGE_CPU_ONLY
		};
		VmaAllocation stagingBufferAllocation;
		VkBuffer stagingBuffer {};
		vmaCreateBuffer(allocator.get(), bufferCreateInfo, &allocationCreateInfo, &stagingBuffer, &stagingBufferAllocation, nullptr);

		void *mappedData;
		vmaMapMemory(allocator.get(), stagingBufferAllocation, &mappedData);
		std::memcpy(mappedData, image->pixels, bufferCreateInfo.size);
		vmaUnmapMemory(allocator.get(), stagingBufferAllocation);

		// Create Image
		vk::ImageCreateInfo imageCreateInfo {
			.imageType = vk::ImageType::e2D,
			.format = vk::Format::eR8G8B8A8Srgb,
			.extent = vk::Extent3D {
				.width = (uint32_t)(image->w),
				.height = (uint32_t)(image->h),
				.depth = 1
			},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = vk::SampleCountFlagBits::e1,
			.tiling = vk::ImageTiling::eOptimal,
			.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferDst,
			.initialLayout = vk::ImageLayout::eUndefined
		};
		VmaAllocation imageAllocation;
		VkImage vkImage;
		VmaAllocationCreateInfo imageAllocationCreateInfo {
			.usage = VMA_MEMORY_USAGE_GPU_ONLY
		};
		auto const rawImageCreateInfo {static_cast<VkImageCreateInfo>(imageCreateInfo)};
		if (vmaCreateImage(allocator.get(), &rawImageCreateInfo, &imageAllocationCreateInfo, &vkImage, &imageAllocation, nullptr) != VK_SUCCESS)
			throw;

		auto const &frame {*frames[0]};
		vk::CommandBufferBeginInfo beginInfo {
			.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
		};

		frame.commandBuffer.begin(beginInfo);
		TransitionImageLayout(frame.commandBuffer, vkImage,
							  ImageLayout {
								  vk::ImageLayout::eUndefined,
								  vk::PipelineStageFlagBits2::eNone,
								  vk::AccessFlagBits2::eNone
							  },
							  ImageLayout {
								  vk::ImageLayout::eTransferDstOptimal,
								  vk::PipelineStageFlagBits2::eTransfer,
								  vk::AccessFlagBits2::eTransferWrite
							  });
		frame.commandBuffer.copyBufferToImage(stagingBuffer, vkImage, vk::ImageLayout::eTransferDstOptimal,
											  vk::BufferImageCopy {
												  .bufferOffset = 0,
												  .bufferRowLength = 0,
												  .bufferImageHeight = 0,
												  .imageSubresource = vk::ImageSubresourceLayers {
													  vk::ImageAspectFlagBits::eColor, 0, 0, 1
												  },
												  .imageOffset = vk::Offset3D {0, 0, 0},
												  .imageExtent = vk::Extent3D {(uint32_t)(image->w), (uint32_t)(image->h), 1}
											  });
		TransitionImageLayout(frame.commandBuffer, vkImage,
							  ImageLayout {
								  vk::ImageLayout::eTransferDstOptimal,
								  vk::PipelineStageFlagBits2::eTransfer,
								  vk::AccessFlagBits2::eTransferWrite
							  },
							  ImageLayout {
								  vk::ImageLayout::eTransferDstOptimal,
								  vk::PipelineStageFlagBits2::eTransfer,
								  vk::AccessFlagBits2::eTransferRead
							  });
		frame.commandBuffer.end();
		device->resetFences(*frame.fence);
		vk::SubmitInfo submitInfo {};
		submitInfo.setCommandBuffers(*frame.commandBuffer);
		graphicsQueue->submit(submitInfo, frame.fence);

		SDL_DestroySurface(image);
		device->waitForFences(*frame.fence, true, UINT64_MAX);

		// cleanup
		vmaDestroyBuffer(allocator.get(), stagingBuffer, stagingBufferAllocation);
		frame.commandBuffer.reset();

		texture.emplace(*device, vkImage);
	}

	void Run() {
		while (!done) {
			HandleEvents();
			Render();
		}
	}

  private:
	void InitVMAAllocator() {
		VmaVulkanFunctions const functions {
			.vkGetInstanceProcAddr = instance->getDispatcher()->vkGetInstanceProcAddr,
			.vkGetDeviceProcAddr = device->getDispatcher()->vkGetDeviceProcAddr
		};

		VmaAllocatorCreateInfo allocatorCreateInfo {
			.physicalDevice = **physicalDevice,
			.device = **device,
			.pVulkanFunctions = &functions,
			.instance = **instance,
			.vulkanApiVersion = vulkanVersion
		};
		VmaAllocator vmaAllocator;
		vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator);
		allocator.reset(vmaAllocator);
	};

	struct ImageLayout {
		vk::ImageLayout imageLayout {};
		vk::PipelineStageFlags2 stageMask {};
		vk::AccessFlags2 accessMask {};
		uint32_t queueFamilyIndex {vk::QueueFamilyIgnored};
	};

	// Unneeded, but to read https://themaister.net/blog/2019/08/14/yet-another-blog-explaining-vulkan-synchronization/
	// vs Vulkan13 sync2
	static void TransitionImageLayoutOld(vk::raii::CommandBuffer const &commandBuffer, vk::Image const &image) {
		vk::ImageMemoryBarrier const barrier {
			.srcAccessMask = vk::AccessFlagBits::eMemoryRead,
			.dstAccessMask = vk::AccessFlagBits::eTransferWrite,
			.oldLayout = vk::ImageLayout::eUndefined,
			.newLayout = vk::ImageLayout::eTransferDstOptimal,
			.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
			.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
			.image = image,
			.subresourceRange = vk::ImageSubresourceRange {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
	}

	static void TransitionImageLayout(vk::raii::CommandBuffer const &commandBuffer, vk::Image const &image,
									  ImageLayout const &oldLayout, ImageLayout const &newLayout) {
		vk::ImageMemoryBarrier2 const barrier {
			.srcStageMask = oldLayout.stageMask,
			.srcAccessMask = oldLayout.accessMask,
			.dstStageMask = newLayout.stageMask,
			.dstAccessMask = newLayout.accessMask,
			.oldLayout = oldLayout.imageLayout,
			.newLayout = newLayout.imageLayout,
			.srcQueueFamilyIndex = oldLayout.queueFamilyIndex,
			.dstQueueFamilyIndex = newLayout.queueFamilyIndex,
			.image = image,
			.subresourceRange = vk::ImageSubresourceRange {
				vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
			}
		};
		vk::DependencyInfo dependencyInfo {};
		dependencyInfo.setImageMemoryBarriers(barrier);
		commandBuffer.pipelineBarrier2(dependencyInfo);
	}

	void RecordCommandBuffer(vk::raii::CommandBuffer const &commandBuffer, vk::Image const &swapchainImage) {
		commandBuffer.reset();
		const vk::CommandBufferBeginInfo beginInfo {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
		commandBuffer.begin(beginInfo);

		double time = SDL_GetTicks() * 0.001;
		const glm::vec4 rainbow = hsv_to_rgb(glm::vec4 {sin(time) * 0.5 + 0.5, 1.0f, 1.0f, 1.0f});
		const vk::ClearColorValue color {rainbow.r, rainbow.g, rainbow.b, rainbow.a};

		TransitionImageLayout(commandBuffer, swapchainImage,
							  ImageLayout {
								  .imageLayout = vk::ImageLayout::eUndefined,
								  .stageMask = vk::PipelineStageFlagBits2::eTransfer,
								  .accessMask = vk::AccessFlagBits2::eMemoryRead
							  },
							  ImageLayout {
								  .imageLayout = vk::ImageLayout::eTransferDstOptimal,
								  .stageMask = vk::PipelineStageFlagBits2::eTransfer,
								  .accessMask = vk::AccessFlagBits2::eTransferWrite
							  });

		commandBuffer.clearColorImage(swapchainImage, vk::ImageLayout::eTransferDstOptimal, color, vk::ImageSubresourceRange {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
		//
		// blit texture to swapchain image
		std::array srcOffsets {
			vk::Offset3D {0, 0, 0},
			vk::Offset3D {(int32_t)textureWidth, (int32_t)textureHeight, 1}
		};
		std::array dstOffsets {
			vk::Offset3D {0, 0, 0},
			vk::Offset3D {(int32_t)swapchainExtent.width, (int32_t)swapchainExtent.height, 1}
		};

		commandBuffer.blitImage(**texture, vk::ImageLayout::eTransferSrcOptimal, swapchainImage, vk::ImageLayout::eTransferDstOptimal,
								vk::ImageBlit {
									.srcSubresource = vk::ImageSubresourceLayers {
										vk::ImageAspectFlagBits::eColor, 0, 0, 1
									},
									.srcOffsets = srcOffsets,
									.dstSubresource = vk::ImageSubresourceLayers {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
									.dstOffsets = dstOffsets
								},
								vk::Filter::eLinear);

		TransitionImageLayout(commandBuffer, swapchainImage,
							  ImageLayout {
								  .imageLayout = vk::ImageLayout::eTransferDstOptimal,
								  .stageMask = vk::PipelineStageFlagBits2::eTransfer,
								  .accessMask = vk::AccessFlagBits2::eTransferWrite,
							  },
							  ImageLayout {
								  .imageLayout = vk::ImageLayout::ePresentSrcKHR,
								  .stageMask = vk::PipelineStageFlagBits2::eTransfer,
								  .accessMask = vk::AccessFlagBits2::eMemoryRead
							  });

		commandBuffer.end();
	}

	void BeginFrame(Frame const &frame) {
		device->waitForFences(*frame.fence, true, UINT64_MAX);
		device->resetFences(*frame.fence);

		// We are violating Vulkan spec here, see https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
		auto [acquireResult, imageIndex] = swapchain->acquireNextImage(UINT64_MAX, *frame.imageAvailableSemaphore, nullptr);
		currentSwapchainImageIndex = imageIndex;
	}

	void EndFrame(Frame const &frame) {
		vk::PresentInfoKHR presentInfo {};
		presentInfo.setSwapchains(**swapchain);
		presentInfo.setImageIndices(currentSwapchainImageIndex);
		presentInfo.setWaitSemaphores(*frame.renderFinishedSemaphore);
		graphicsQueue->presentKHR(presentInfo);

		frameIndex = (frameIndex + 1) % IN_FLIGHT_FRAME_COUNT;
	}
	void SubmitCommandBuffer(Frame const &frame) {
		vk::SubmitInfo submitInfo {};
		submitInfo.setCommandBuffers(*frame.commandBuffer);
		submitInfo.setWaitSemaphores(*frame.imageAvailableSemaphore);
		submitInfo.setSignalSemaphores(*frame.renderFinishedSemaphore);
		constexpr vk::PipelineStageFlags waitStage {vk::PipelineStageFlagBits::eTransfer};
		submitInfo.setWaitDstStageMask(waitStage);
		graphicsQueue->submit(submitInfo, frame.fence);
	}

	void Render() {
		Frame const &frame {*frames[frameIndex]};

		BeginFrame(frame);
		RecordCommandBuffer(frame.commandBuffer, swapchainImages[currentSwapchainImageIndex]);
		SubmitCommandBuffer(frame);
		EndFrame(frame);
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

	void InitFrames() {
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo {
			.commandPool = *commandPool,
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = IN_FLIGHT_FRAME_COUNT
		};
		auto commandBuffers {device->allocateCommandBuffers(commandBufferAllocateInfo)};

		for (size_t i = 0; i < IN_FLIGHT_FRAME_COUNT; i++)
			frames[i].emplace(
				std::move(commandBuffers[i]),
				vk::raii::Semaphore {*device, vk::SemaphoreCreateInfo {}},
				vk::raii::Semaphore {*device, vk::SemaphoreCreateInfo {}},
				vk::raii::Fence {*device, vk::FenceCreateInfo {.flags = vk::FenceCreateFlagBits::eSignaled}});
	}

	void InitCommandPool() {
		vk::CommandPoolCreateInfo commandPoolCreateInfo {};
		commandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
		commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		commandPool.emplace(*device, commandPoolCreateInfo);
	}

	void InitInstance() {
		vk::ApplicationInfo applicationinfo {.apiVersion = vulkanVersion};
		vk::InstanceCreateInfo instanceCreateInfo {.pApplicationInfo = &applicationinfo};

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

		vk::PhysicalDeviceVulkan13Features vulkan13Features {.synchronization2 = true};
		vk::StructureChain chain {deviceCreateInfo, vulkan13Features};

		device.emplace(*physicalDevice, chain.get<vk::DeviceCreateInfo>());

		graphicsQueue.emplace(*device, graphicsQueueFamilyIndex, 0);
	}
};
