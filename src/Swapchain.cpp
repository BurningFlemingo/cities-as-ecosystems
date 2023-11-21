#include "Swapchain.h"
#include <SDL_vulkan.h>
#include <algorithm>

#include <stdexcept>

VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& avaliableFormats);
VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
VkExtent2D chooseSwapchainExtent(SDL_Window* window, VkSurfaceCapabilitiesKHR capabilities);

Swapchain createSwapchain(
		VkDevice device,
		SDL_Window* window,
		VkSurfaceKHR surface,
		const SurfaceSupportDetails& surfaceSupportDetails,
		const std::vector<uint32_t>& queueFamilyIndices
		) {
	VkSurfaceFormatKHR surfaceFormat = chooseSwapchainSurfaceFormat(surfaceSupportDetails.surfaceFormats);

	Swapchain swapchain{};
	swapchain.format = surfaceFormat.format;
	swapchain.colorSpace = surfaceFormat.colorSpace;
	swapchain.presentMode = chooseSwapchainPresentMode(surfaceSupportDetails.presentModes);
	swapchain.extent = chooseSwapchainExtent(window, surfaceSupportDetails.surfaceCapabilities);

	uint32_t imageCount{};
	imageCount = surfaceSupportDetails.surfaceCapabilities.minImageCount + 1;
	if (imageCount > surfaceSupportDetails.surfaceCapabilities.maxImageCount && surfaceSupportDetails.surfaceCapabilities.maxImageCount > 0) {
		imageCount = surfaceSupportDetails.surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.presentMode = swapchain.presentMode;
	swapchainCreateInfo.imageColorSpace = swapchain.colorSpace;
	swapchainCreateInfo.imageFormat = swapchain.format;
	swapchainCreateInfo.imageExtent = swapchain.extent;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.queueFamilyIndexCount = queueFamilyIndices.size();
	swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.preTransform = surfaceSupportDetails.surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain.handle) != VK_SUCCESS) {
		throw std::runtime_error("could not create swapchain");
	}

	return swapchain;
}

std::vector<VkImageView> createSwapchainImageViews(
		VkDevice device,
		const Swapchain& swapchain) {
	uint32_t swapchainImageCount{};
	vkGetSwapchainImagesKHR(device, swapchain.handle, &swapchainImageCount, nullptr);

	std::vector<VkImage> swapchainImages(swapchainImageCount);
	vkGetSwapchainImagesKHR(device, swapchain.handle, &swapchainImageCount, swapchainImages.data());

	std::vector<VkImageView> swapchainImageViews;
	for (const auto& image : swapchainImages) {
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.format = swapchain.format;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageView imageView{};
		vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView);
		swapchainImageViews.emplace_back(imageView);
	}

	return swapchainImageViews;
}

std::vector<VkFramebuffer> createSwapchainFramebuffers(
		VkDevice device,
		const std::vector<VkImageView>& swapchainImageViews,
		const VkExtent2D swapchainExtent,
		const VkRenderPass renderPass) {
	std::vector<VkFramebuffer> swapchainFramebuffers(swapchainImageViews.size());
	for (int i{}; i < swapchainFramebuffers.size(); i++) {
		VkImageView attachment{swapchainImageViews[i]};

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.width = swapchainExtent.width;
		framebufferCreateInfo.height = swapchainExtent.height;
		framebufferCreateInfo.renderPass = renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &attachment;
		framebufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("could not create framebuffer");
		}
	}

	return swapchainFramebuffers;
}

VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& avaliableFormats) {
	VkSurfaceFormatKHR bestFormat{};
	int bestFormatRating{-1};

	for (const auto& format : avaliableFormats) {
		int thisFormatRating{};
		if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			thisFormatRating += 10;
		}
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {
			thisFormatRating += 100;
		}

		if (thisFormatRating > bestFormatRating) {
			bestFormat = format;
			bestFormatRating = thisFormatRating;
		}
	}
	return bestFormat;
}

VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& presentModes) {
	for (const auto& presentMode : presentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapchainExtent(SDL_Window* window, VkSurfaceCapabilitiesKHR capabilities) {
	int width{};	
	int height{};	
	SDL_Vulkan_GetDrawableSize(window, &width, &height);

	VkExtent2D windowExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	if (capabilities.maxImageExtent.width > 0) {
		windowExtent.width = std::clamp(
				windowExtent.width, 
				capabilities.minImageExtent.width,
				capabilities.maxImageExtent.width);
	} else {
		windowExtent.width = std::max(windowExtent.width, 
				capabilities.minImageExtent.width);
	}

	if (capabilities.maxImageExtent.height > 0) {
		windowExtent.height = std::clamp(
				windowExtent.height, 
				capabilities.minImageExtent.height,
				capabilities.maxImageExtent.height);
	} else {
		windowExtent.height = std::max(windowExtent.height, 
				capabilities.minImageExtent.height);
	}

	return windowExtent;
}
