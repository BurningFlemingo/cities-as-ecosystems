#pragma once

#include <vulkan/vulkan.h>

struct VulkanContext;

#include <vector>

#include "Cleanup.h"
// class DeletionQueue;

// forward declerations
struct Device;
typedef struct SDL_Window SDL_Window;

struct Swapchain {
	VkSwapchainKHR handle;

	VkFormat format;
	VkExtent2D extent;

	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
};

namespace VulkanRenderer {
	// will call flush to ensure only 1 swapchain per window.
	// use a deticated swapchain deletion queue
	Deletable<Swapchain> createSwapchain(
		const VulkanContext& ctx,
		SDL_Window* window,
		const VkSwapchainKHR oldSwapchainHandle = VK_NULL_HANDLE
	);
}  // namespace VulkanRenderer
