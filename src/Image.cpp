#define STB_IMAGE_IMPLEMENTATION

#include "Image.h"

#include "debug/Debug.h"
#include "CommandUtils.h"
#include "MemoryUtils.h"

#include "FileIO.h"

namespace VkUtils {

void createImage(
		const Device& device,
		uint32_t width, uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags memProperties,
		VkImage* outImage, VkDeviceMemory* outImageMemory
	) {
	VkImageCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.extent.width = width;
	createInfo.extent.height = height;
	createInfo.extent.depth = 1;
	createInfo.mipLevels = 1;
	createInfo.arrayLayers = 1;

	createInfo.format = format;
	createInfo.tiling = tiling;

	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	createInfo.usage = usage;

	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	if (vkCreateImage(device.logical, &createInfo, nullptr, outImage) != VK_SUCCESS) {
		throw std::runtime_error("could not create texture image");
	}

	VkMemoryRequirements memRequirements{};
	vkGetImageMemoryRequirements(device.logical, *outImage, &memRequirements);

	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = findValidMemoryTypeIndex(device, memRequirements.memoryTypeBits, memProperties);

	if (vkAllocateMemory(device.logical, &memAllocInfo, nullptr, outImageMemory) != VK_SUCCESS) {
		throw std::runtime_error("could not allocate memory");
	}

	vkBindImageMemory(device.logical, *outImage, *outImageMemory, 0);
}

void copyBufferToImage(
		const Device& device,
		VkCommandPool commandPool,
		VkQueue transferQueue,
		VkBuffer buffer,
		VkImage image,
		uint32_t width,
		uint32_t height
	) {
	VkCommandBuffer cmdBuffer{beginTransientCommands(device, commandPool)};

	VkBufferImageCopy region{};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endTransientCommands(device, commandPool, cmdBuffer, transferQueue);
}

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
	) {
	VkCommandBuffer cmdBuffer{beginTransientCommands(device, transferPool)};

	VkImageMemoryBarrier barrier{};

	VkPipelineStageFlags srcStage{};
	VkPipelineStageFlags dstStage{};
	VkAccessFlags srcAccessFlags{};
	VkAccessFlags dstAccessFlags{};

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		srcAccessFlags = VK_ACCESS_HOST_WRITE_BIT;
		dstAccessFlags = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		srcAccessFlags = VK_ACCESS_TRANSFER_WRITE_BIT;
		dstAccessFlags = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		throw std::invalid_argument("unsupported layout transitions");
	}

	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
	barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (srcQueueFamilyIndex != dstQueueFamilyIndex) {
		barrier.srcAccessMask = srcAccessFlags;
		barrier.dstAccessMask = 0;
		vkCmdPipelineBarrier(
				cmdBuffer,
				srcStage, srcStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

		VkSemaphoreCreateInfo semCreateInfo{};
		semCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkSemaphore ownershipTransitionSem{};
		vkCreateSemaphore(device.logical, &semCreateInfo, nullptr, &ownershipTransitionSem);

		vkEndCommandBuffer(cmdBuffer);

		VkSubmitInfo releaseInfo{};
		releaseInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		releaseInfo.commandBufferCount = 1;
		releaseInfo.pCommandBuffers = &cmdBuffer;
		releaseInfo.signalSemaphoreCount = 1;
		releaseInfo.pSignalSemaphores = &ownershipTransitionSem;

		VkCommandBuffer aquireCmdBuffer{};

		VkSubmitInfo aquireInfo{};
		aquireInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		aquireInfo.commandBufferCount = 1;
		aquireInfo.pCommandBuffers = &aquireCmdBuffer;
		aquireInfo.waitSemaphoreCount = 1;
		aquireInfo.pWaitSemaphores = &ownershipTransitionSem;
		aquireInfo.pWaitDstStageMask = &srcStage;

		aquireCmdBuffer = beginTransientCommands(device, graphicsPool);

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = dstAccessFlags;
		vkCmdPipelineBarrier(
				aquireCmdBuffer,
				srcStage, dstStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

		vkEndCommandBuffer(aquireCmdBuffer);

		vkQueueSubmit(transferQueue, 1, &releaseInfo, VK_NULL_HANDLE);
		vkQueueSubmit(graphicsQueue, 1, &aquireInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);
		vkDestroySemaphore(device.logical, ownershipTransitionSem, nullptr);
	} else {
		vkCmdPipelineBarrier(
				cmdBuffer,
				srcStage, dstStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

		endTransientCommands(device, transferPool, cmdBuffer, transferQueue);
	}
}

VkImageView createImageView(const Device& device, VkImage image, VkFormat format) {
	VkImageView imageView{};

	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device.logical, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS) {
		assert("could not create image view");
	}

	return imageView;
}

VkSampler createSampler(const Device& device) {
	VkSampler sampler{};

	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;

	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.mipLodBias = 0;
	samplerCreateInfo.anisotropyEnable = device.features.samplerAnisotropy;
	samplerCreateInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.minLod = 0;
	samplerCreateInfo.maxLod = 0;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	if (vkCreateSampler(device.logical, &samplerCreateInfo, nullptr, &sampler) != VK_SUCCESS) {
		logWarning("could not create sampler");
	}

	return sampler;
}

ImageInfo loadRGBAImage(const std::string& filePath) {
	ImageInfo iInfo{};
	stbi_set_flip_vertically_on_load(true);
	stbi_uc* pixels{stbi_load(filePath.c_str(), &iInfo.width, &iInfo.height, &iInfo.channels, STBI_rgb_alpha)};
	iInfo.pixels = pixels;

	if (!pixels) {
		logWarning("could not load image");
	}

	iInfo.bytesPerPixel = sizeof(uint8_t) * 4;

	return iInfo;
}

void freeImage(ImageInfo* imageInfo) {
	stbi_image_free(imageInfo->pixels);
	imageInfo->pixels = nullptr;
}

void createTextureImage(
		const Device& device,
		VkCommandPool transferPool,
		VkCommandPool graphicsPool,
		VkQueue transferQueue,
		VkQueue graphicsQueue,
		const std::string& filePath,
		VkImage* textureImage, VkDeviceMemory* textureMemory
	) {

	ImageInfo imageInfo{};
	VkBuffer stagingBuffer{};
	VkDeviceMemory stagingBufferMemory{};
	{
		imageInfo = loadRGBAImage(filePath);
		auto imageSize{static_cast<VkDeviceSize>(imageInfo.width * imageInfo.height * imageInfo.bytesPerPixel)};

		createBuffer(
				device,
				imageSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				VK_SHARING_MODE_EXCLUSIVE,
				&stagingBuffer, 
				&stagingBufferMemory
		);

		void* mappedMem{};
		vkMapMemory(device.logical, stagingBufferMemory, 0, imageSize, 0, &mappedMem);
		memcpy(mappedMem, imageInfo.pixels, imageSize);
		vkUnmapMemory(device.logical, stagingBufferMemory);

		freeImage(&imageInfo);
	}

	createImage(
			device,
			imageInfo.width, imageInfo.height,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			textureImage, textureMemory
		);

	uint32_t transferQueueFamilyIndex{device.queueFamilyIndices.transferIndex};
	uint32_t graphicsQueueFamilyIndex{device.queueFamilyIndices.graphicsIndex};

	transitionImageLayout(
			device,
			transferPool, graphicsPool,
			transferQueue, graphicsQueue,
			*textureImage,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			VK_FORMAT_R8G8B8A8_SRGB
		);
	copyBufferToImage(
			device,
			transferPool,
			transferQueue,
			stagingBuffer,
			*textureImage,
			imageInfo.width, imageInfo.height
		);
	transitionImageLayout(
			device,
			transferPool, graphicsPool,
			transferQueue, graphicsQueue,
			*textureImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			transferQueueFamilyIndex, graphicsQueueFamilyIndex,
			VK_FORMAT_R8G8B8A8_SRGB
		);

	vkDestroyBuffer(device.logical, stagingBuffer, nullptr);
	vkFreeMemory(device.logical, stagingBufferMemory, nullptr);
}

}
