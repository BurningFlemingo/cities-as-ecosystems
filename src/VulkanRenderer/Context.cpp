#include "Context.h"

#include "Cleanup.h"
#include "Device.h"
#include "Instance.h"
#include "RendererPCH.h"
#include "debug/Debug.h"

#include <SDL2/SDL.h>

namespace {
	VmaAllocator createAllocator(
		const Device& device,
		const VkInstance& instance,
		DeletionQueue& deletionQueue
	);
	VkSurfaceKHR createSurface(
		SDL_Window* window,
		const VkInstance& instance,
		DeletionQueue& deletionQueue
	);
}  // namespace

VulkanContext VulkanRenderer::createVulkanContext(
	SDL_Window* window, DeletionQueue& deletionQueue
) {
	auto [instance, debugMessenger]{ createInstance(window, deletionQueue) };

	VkSurfaceKHR surface{ createSurface(window, instance, deletionQueue) };

	Device device{ createDevice(surface, instance, deletionQueue) };

	VmaAllocator allocator{ createAllocator(device, instance, deletionQueue) };

	VulkanContext context{
		.allocator = allocator,

		.instance = instance,
		.debugMessenger = debugMessenger,

		.surface = surface,

		.device = device,
	};

	return context;
}

namespace {
	VmaAllocator createAllocator(
		const Device& device,
		const VkInstance& instance,
		DeletionQueue& deletionQueue
	) {
		VmaAllocatorCreateInfo allocatorCreateInfo{
			.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			.physicalDevice = device.physical,
			.device = device.logical,
			.instance = instance,
		};

		VmaAllocator allocator{};
		if (vmaCreateAllocator(&allocatorCreateInfo, &allocator)
			!= VK_SUCCESS) {
			logFatal("could not create vma allocator");
		}

		deletionQueue.pushFunction([=]() {
			vmaDestroyAllocator(allocator);
		});

		return allocator;
	}

	VkSurfaceKHR createSurface(
		SDL_Window* window,
		const VkInstance& instance,
		DeletionQueue& deletionQueue
	) {
		VkSurfaceKHR surface{};
		SDL_Vulkan_CreateSurface(window, instance, &surface);

		deletionQueue.pushFunction([=]() {
			vkDestroySurfaceKHR(instance, surface, nullptr);
		});

		return surface;
	}
}  // namespace
