#include "Swapchain.h"
#include <SDL_vulkan.h>
#include <algorithm>

#include <stdexcept>

VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& avaliableFormats);
VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
VkExtent2D chooseSwapchainExtent(SDL_Window* window, VkSurfaceCapabilitiesKHR capabilities);

struct SwapchainInfo {
	VkPresentModeKHR presentMode{};
	VkColorSpaceKHR colorSpace{};
	VkFormat format{};
	VkExtent2D extent{};

	uint32_t imageCount{};
};

SwapchainInfo fillSwapchainInfo(SDL_Window* window, const SurfaceSupportDetails& surfaceSupportDetails) {
	SwapchainInfo swapchainInfo{};

	VkSurfaceFormatKHR surfaceFormat = chooseSwapchainSurfaceFormat(surfaceSupportDetails.surfaceFormats);

	swapchainInfo.format = surfaceFormat.format;
	swapchainInfo.colorSpace = surfaceFormat.colorSpace;
	swapchainInfo.presentMode = chooseSwapchainPresentMode(surfaceSupportDetails.presentModes);
	swapchainInfo.extent = chooseSwapchainExtent(window, surfaceSupportDetails.surfaceCapabilities);

	uint32_t& imageCount{swapchainInfo.imageCount};
	imageCount = surfaceSupportDetails.surfaceCapabilities.minImageCount + 1;
	if (imageCount > surfaceSupportDetails.surfaceCapabilities.maxImageCount && surfaceSupportDetails.surfaceCapabilities.maxImageCount > 0) {
		imageCount = surfaceSupportDetails.surfaceCapabilities.maxImageCount;
	}

	return swapchainInfo;
}

Swapchain createSwapchain(
		VkDevice device,
		SDL_Window* window,
		VkSurfaceKHR surface,
		const SurfaceSupportDetails& surfaceSupportDetails) {

	Swapchain swapchain{};

	SwapchainInfo swapchainInfo{fillSwapchainInfo(window, surfaceSupportDetails)};


	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.presentMode = swapchainInfo.presentMode;
	swapchainCreateInfo.imageColorSpace = swapchainInfo.colorSpace;
	swapchainCreateInfo.imageFormat = swapchainInfo.format;
	swapchainCreateInfo.imageExtent = swapchainInfo.extent;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = swapchainInfo.imageCount;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.preTransform = surfaceSupportDetails.surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain.handle) != VK_SUCCESS) {
		throw std::runtime_error("could not create swapchain");
	}

	swapchain.presentMode = swapchainInfo.presentMode;
	swapchain.colorSpace = swapchainInfo.colorSpace;
	swapchain.format = swapchainInfo.format;
	swapchain.extent = swapchainInfo.extent;

	return swapchain;
}

Swapchain recreateSwapchain(VkDevice device,
		SDL_Window* window,
		VkSurfaceKHR surface,
		const SurfaceSupportDetails& surfaceSupportDetails, 
		VkSwapchainKHR oldSwapchainHandle) {
	Swapchain swapchain{};

	SwapchainInfo swapchainInfo{fillSwapchainInfo(window, surfaceSupportDetails)};

	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.presentMode = swapchainInfo.presentMode;
	swapchainCreateInfo.imageColorSpace = swapchainInfo.colorSpace;
	swapchainCreateInfo.imageFormat = swapchainInfo.format;
	swapchainCreateInfo.imageExtent = swapchainInfo.extent;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = swapchainInfo.imageCount;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.preTransform = surfaceSupportDetails.surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.oldSwapchain = oldSwapchainHandle;

	if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain.handle) != VK_SUCCESS) {
		throw std::runtime_error("could not create swapchain");
	}

	swapchain.presentMode = swapchainInfo.presentMode;
	swapchain.colorSpace = swapchainInfo.colorSpace;
	swapchain.format = swapchainInfo.format;
	swapchain.extent = swapchainInfo.extent;

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

void destroySwapchainFramebuffers(VkDevice device, std::vector<VkFramebuffer>* framebuffers) {
	for (auto framebuffer : *framebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
		framebuffer = {};
	}
	framebuffers->clear();
}

void destroySwapchainImageViews(VkDevice device, std::vector<VkImageView>* imageViews, std::vector<VkFramebuffer>* framebuffers) {
	if (framebuffers) {
		destroySwapchainFramebuffers(device, framebuffers);
	}
	for (auto& imageView : *imageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}
	imageViews->clear();
}

void destroySwapchain(VkDevice device, VkSwapchainKHR swapchainHandle, std::vector<VkImageView>* imageViews, std::vector<VkFramebuffer>* framebuffers) {
	if (imageViews) {
		destroySwapchainImageViews(device, imageViews, framebuffers);
	}
	vkDestroySwapchainKHR(device, swapchainHandle, nullptr);
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


