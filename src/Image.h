#pragma once

#include "pch.h"
#include <stb/stb_image.h>

#include "Device.h"
#include "Buffer.h"

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

VkImageView createImageView(const Device& device, VkImage image, VkFormat format);
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
