#include "Swapchain.h"

#include "Cleanup.h"
#include "Context.h"
#include "Device.h"
#include "RendererPCH.h"
#include "debug/Debug.h"
#include "vkutils/Device.h"

#include <algorithm>

namespace {
	struct SwapchainInfo {
		VkPresentModeKHR presentMode;
		VkColorSpaceKHR colorSpace;
		VkFormat format;
		VkExtent2D extent;

		uint32_t imageCount;
	};

	SwapchainInfo fillSwapchainInfo(
		SDL_Window* window,
		const vkutils::SurfaceSupportDetails& surfaceSupportDetails
	);

	VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats
	);
	VkPresentModeKHR chooseSwapchainPresentMode(
		const std::vector<VkPresentModeKHR>& presentModes
	);
	VkExtent2D chooseSwapchainExtent(
		SDL_Window* window, VkSurfaceCapabilitiesKHR capabilities
	);
	uint32_t chooseImageCount(
		const vkutils::SurfaceSupportDetails& surfaceSupportDetails
	);

	void destroySwapchainImageViews(
		const Device& device, std::vector<VkImageView>* imageViews
	);

	std::vector<VkImageView> createSwapchainImageViews(
		const Device& device,
		const std::vector<VkImage>& swapchainImages,
		const VkSwapchainKHR swapchainHandle,
		const VkFormat& swapchainFormat
	);
}  // namespace

Deletable<Swapchain> VulkanRenderer::createSwapchain(
	const VulkanContext& ctx,
	SDL_Window* window,
	const VkSwapchainKHR oldSwapchainHandle
) {
	Swapchain swapchain{};

	vkutils::SurfaceSupportDetails surfaceSupportDetails{
		vkutils::queryDeviceSurfaceSupportDetails(
			ctx.device.physical, ctx.surface
		)
	};
	SwapchainInfo swapchainInfo{
		fillSwapchainInfo(window, surfaceSupportDetails)
	};

	VkSwapchainCreateInfoKHR swapchainCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = ctx.surface,
		.minImageCount = swapchainInfo.imageCount,
		.imageFormat = swapchainInfo.format,
		.imageColorSpace = swapchainInfo.colorSpace,
		.imageExtent = swapchainInfo.extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform =
			surfaceSupportDetails.surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = swapchainInfo.presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = oldSwapchainHandle,
	};

	if (vkCreateSwapchainKHR(
			ctx.device.logical, &swapchainCreateInfo, nullptr, &swapchain.handle
		) != VK_SUCCESS) {
		logFatal("could not create swapchain");
	}

	swapchain.format = swapchainInfo.format;
	swapchain.extent = swapchainInfo.extent;

	{
		uint32_t swapchainImageCount{};
		vkGetSwapchainImagesKHR(
			ctx.device.logical, swapchain.handle, &swapchainImageCount, nullptr
		);

		std::vector<VkImage> swapchainImages(swapchainImageCount);
		vkGetSwapchainImagesKHR(
			ctx.device.logical,
			swapchain.handle,
			&swapchainImageCount,
			swapchainImages.data()
		);
		swapchain.images = swapchainImages;
	}

	swapchain.imageViews = createSwapchainImageViews(
		ctx.device, swapchain.images, swapchain.handle, swapchain.format
	);

	auto deleter{ [=]() {
		for (auto& imageView : swapchain.imageViews) {
			vkDestroyImageView(ctx.device.logical, imageView, nullptr);
		}
		vkDestroySwapchainKHR(ctx.device.logical, swapchain.handle, nullptr);
	} };

	return { .obj = swapchain, .deleter = deleter };
}

void destroySwapchain(const Device& device, Swapchain* swapchain) {
	vkDestroySwapchainKHR(device.logical, swapchain->handle, nullptr);
	destroySwapchainImageViews(device, &swapchain->imageViews);
}

namespace {
	SwapchainInfo fillSwapchainInfo(
		SDL_Window* window,
		const vkutils::SurfaceSupportDetails& surfaceSupportDetails
	) {
		VkSurfaceFormatKHR surfaceFormat{
			chooseSwapchainSurfaceFormat(surfaceSupportDetails.surfaceFormats)
		};
		VkPresentModeKHR presentMode{
			chooseSwapchainPresentMode(surfaceSupportDetails.presentModes)
		};
		VkExtent2D extent{ chooseSwapchainExtent(
			window, surfaceSupportDetails.surfaceCapabilities
		) };
		uint32_t imageCount{ chooseImageCount(surfaceSupportDetails) };

		SwapchainInfo swapchainInfo{ .presentMode = presentMode,
									 .colorSpace = surfaceFormat.colorSpace,
									 .format = surfaceFormat.format,
									 .extent = extent,
									 .imageCount = imageCount };

		return swapchainInfo;
	}

	VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats
	) {
		VkSurfaceFormatKHR surfaceFormat{
			.format = VK_FORMAT_B8G8R8A8_SRGB,
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};

		bool foundFormat{};
		bool foundColorSpace{};

		for (const auto& format : availableSurfaceFormats) {
			if (format.format == surfaceFormat.format) {
				foundFormat = true;
			}
			if (format.colorSpace == surfaceFormat.colorSpace) {
				foundColorSpace = true;
			}
		}

		if (!foundFormat || !foundColorSpace) {
			logFatal("could not find required surface format");
		}

		return surfaceFormat;
	}

	VkPresentModeKHR chooseSwapchainPresentMode(
		const std::vector<VkPresentModeKHR>& presentModes
	) {
		for (const auto& presentMode : presentModes) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return presentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapchainExtent(
		SDL_Window* window, VkSurfaceCapabilitiesKHR capabilities
	) {
		int width{};
		int height{};
		SDL_Vulkan_GetDrawableSize(window, &width, &height);

		VkExtent2D windowExtent{ static_cast<uint32_t>(width),
								 static_cast<uint32_t>(height) };

		constexpr uint32_t swapchainSurfaceExtentUndefined{ UINT32_MAX };
		if (capabilities.currentExtent.width !=
			swapchainSurfaceExtentUndefined) {
			windowExtent = capabilities.currentExtent;

		} else {
			windowExtent.width = std::clamp(
				windowExtent.width,
				capabilities.minImageExtent.width,
				capabilities.maxImageExtent.width
			);

			windowExtent.height = std::clamp(
				windowExtent.height,
				capabilities.minImageExtent.height,
				capabilities.maxImageExtent.height
			);
		}

		return windowExtent;
	}

	uint32_t chooseImageCount(
		const vkutils::SurfaceSupportDetails& surfaceSupportDetails
	) {
		uint32_t imageCount{};

		imageCount =
			surfaceSupportDetails.surfaceCapabilities.minImageCount + 1;
		if (imageCount >
				surfaceSupportDetails.surfaceCapabilities.maxImageCount &&
			surfaceSupportDetails.surfaceCapabilities.maxImageCount > 0) {
			imageCount =
				surfaceSupportDetails.surfaceCapabilities.maxImageCount;
		}

		return imageCount;
	}

	void destroySwapchainImageViews(
		const Device& device, std::vector<VkImageView>* imageViews
	) {
		for (auto& imageView : *imageViews) {
			vkDestroyImageView(device.logical, imageView, nullptr);
		}
		imageViews->clear();
	}

	std::vector<VkImageView> createSwapchainImageViews(
		const Device& device,
		const std::vector<VkImage>& swapchainImages,
		const VkSwapchainKHR swapchainHandle,
		const VkFormat& swapchainFormat
	) {
		std::vector<VkImageView> swapchainImageViews;
		for (const auto& image : swapchainImages) {
			VkImageViewCreateInfo imageViewCreateInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = swapchainFormat,
	
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				}, 
			};

			VkImageView imageView{};
			if (vkCreateImageView(
					device.logical, &imageViewCreateInfo, nullptr, &imageView
				) != VK_SUCCESS) {
				logFatal("could not create swapchain image view");
			}
			swapchainImageViews.emplace_back(imageView);
		}

		return swapchainImageViews;
	}
}  // namespace
