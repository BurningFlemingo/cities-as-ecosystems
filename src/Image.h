#pragma once

#include "pch.h"
#include <stb/stb_image.h>

#include "Device.h"
#include "Buffer.h"

namespace VkUtils {

struct ImageInfo {
	int width{};
	int height{};
	int channels{};
	
	uint32_t bytesPerPixel{};

	stbi_uc* pixels{};
};

struct Image {
	VkImage handle{};
	VkDeviceMemory memory{};
	VkImageView view{};
};

void createImage(
		const Device& device,
		uint32_t width, uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags memProperties,
		VkImage* outImage, VkDeviceMemory* outImageMemory
	);

// image must be in optimal layout
void transitionImageLayout(
    const Device& device,
    VkCommandPool transferPool,
    VkCommandPool graphicsPool,
    VkQueue transferQueue,
    VkQueue graphicsQueue,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    uint32_t srcQueueFamilyIndex,
    uint32_t dstQueueFamilyIndex,
    VkFormat format
);

void createTextureImage(
    const Device& device,
    VkCommandPool transferPool,
    VkCommandPool graphicsPool,
    VkQueue transferQueue,
    VkQueue graphicsQueue,
    const std::string& filePath,
    VkImage* textureImage, VkDeviceMemory* textureMemory
);

VkImageView createImageView(const Device& device, VkImage image, VkFormat format, VkImageAspectFlagBits aspect);
VkSampler createSampler(const Device& device);

ImageInfo loadRGBAImage(const std::string& filePath);
void freeImage(ImageInfo* imageInfo);

void copyBufferToImage(
		const Device& device,
		VkCommandPool commandPool,
		VkQueue transferQueue,
		VkBuffer buffer,
		VkImage image,
		uint32_t width,
		uint32_t height
	);

}
