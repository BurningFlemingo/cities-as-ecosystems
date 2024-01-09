#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

// forward declerations
struct VulkanContext;
struct Swapchain;

class DeletionQueue;

struct Image {
	VkImage handle{};
	VkImageView view{};
	VmaAllocation allocation{};

	VkExtent3D extent{};
	VkFormat format{};
};

namespace VulkanRenderer {
	Image createDrawImage(
		const VulkanContext& ctx,
		const Swapchain& swapchain,
		DeletionQueue& deletionQueue
	);
}

namespace vkutils {
	void cmdTransitionImage(
		const VulkanContext& ctx,
		VkCommandBuffer cmdBuffer,
		VkImage image,
		VkImageAspectFlags imageAspectFlags,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		uint32_t srcQueueFamilyIndex,
		uint32_t dstQueueFamilyIndex
	);
}

// struct ImageInfo {
// 	int width{};
// 	int height{};
// 	int channels{};
//
// 	uint32_t bytesPerPixel{};
//
// 	stbi_uc* pixels{};
// };
//
// struct Image {
// 	VkImage handle{};
// 	VkImageView view{};
// 	VkDeviceMemory memory{};
// };
//
// namespace vkutils {
// 	void createImage(
// 			const VulkanContext& ctx,
// 			uint32_t width, uint32_t height,
// 			VkFormat format,
// 			VkImageTiling tiling,
// 			VkImageUsageFlags usage,
// 			VkMemoryPropertyFlags memProperties,
// 			VkImage* outImage, VkDeviceMemory* outImageMemory
// 		);
// 	void transitionImageLayout(
// 		const VulkanContext& ctx,
// 	    VkImage image,
// 	    VkImageLayout oldLayout,
// 	    VkImageLayout newLayout,
// 	    uint32_t srcQueueFamilyIndex,
// 	    uint32_t dstQueueFamilyIndex,
// 	    VkFormat format
// 	);
//
// 	void createTextureImage(
// 		const VulkanContext& ctx,
// 	    const std::string& filePath,
// 	    VkImage* textureImage, VkDeviceMemory* textureMemory
// 	);
//
// 	VkImageView createImageView(const Device& device, VkImage image, VkFormat
// format, VkImageAspectFlagBits aspect); 	VkSampler createSampler(const
// Device& device);
//
// 	ImageInfo loadRGBAImage(const std::string& filePath);
// 	void freeImage(ImageInfo* imageInfo);
//
// 	void copyBufferToImage(
// 			const Device& device,
// 			VkCommandPool commandPool,
// 			VkQueue transferQueue,
// 			VkBuffer buffer,
// 			VkImage image,
// 			uint32_t width,
// 			uint32_t height
// 		);
// }
