#include "RendererPCH.h"

#include "DefaultCreateInfos.h"

VkImageSubresourceRange
	vkdefaults::subresourceRange(const VkImageAspectFlags aspectFlags) {
	VkImageSubresourceRange subresourceRange{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1,
	};

	return subresourceRange;
}

VkCommandBufferBeginInfo
	vkdefaults::commandBufferBeginInfo(const VkCommandBufferUsageFlags flags) {
	VkCommandBufferBeginInfo info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = flags,
	};

	return info;
}

VkSemaphoreSubmitInfo vkdefaults::semSubmitInfo(
	const VkSemaphore semaphore, const VkPipelineStageFlags2 stage
) {
	VkSemaphoreSubmitInfo info{ .sType =
									VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
								.semaphore = semaphore,
								.value = 1,
								.stageMask = stage };

	return info;
}

VkCommandBufferSubmitInfo
	vkdefaults::cmdBufferSubmitInfo(const VkCommandBuffer cmdBuffer) {
	VkCommandBufferSubmitInfo info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = cmdBuffer,
	};

	return info;
}

VkSubmitInfo2 vkdefaults::submitInfo(
	const std::span<const VkCommandBufferSubmitInfo>& cmdBufferInfos,
	const std::span<const VkSemaphoreSubmitInfo>& semWaitInfos,
	const std::span<const VkSemaphoreSubmitInfo>& semSignalInfos
) {
	VkSubmitInfo2 info{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.waitSemaphoreInfoCount = (uint32_t)semWaitInfos.size(),
		.pWaitSemaphoreInfos = semWaitInfos.data(),
		.commandBufferInfoCount = (uint32_t)cmdBufferInfos.size(),
		.pCommandBufferInfos = cmdBufferInfos.data(),
		.signalSemaphoreInfoCount = (uint32_t)semSignalInfos.size(),
		.pSignalSemaphoreInfos = semSignalInfos.data(),
	};

	return info;
}

VkImageCreateInfo vkdefaults::imageCreateInfo(
	const VkExtent3D& extent,
	const VkFormat format,
	const VkImageUsageFlags usage
) {
	VkImageCreateInfo info{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = extent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	return info;
}

VkImageViewCreateInfo vkdefaults::imageViewCreateInfo(
	const VkImage image, const VkFormat format, const VkImageAspectFlags aspect
) {
	VkImageViewCreateInfo info{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = vkdefaults::subresourceRange(aspect),
	};

	return info;
}

VkImageBlit2 vkdefaults::blitRegion(
	const VkExtent3D srcExtent, const VkExtent3D dstExtent
) {
	VkImageBlit2 region{
        .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .srcSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .srcOffsets = {{},
                       {
                           .x = (int32_t)srcExtent.width,
                           .y = (int32_t)srcExtent.height,
                           .z = (int32_t)srcExtent.depth,
                       }},
        .dstSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .dstOffsets = {{},
                       {
                           .x = (int32_t)dstExtent.width,
                           .y = (int32_t)dstExtent.height,
                           .z = (int32_t)dstExtent.depth,
                       }},
    };

	return region;
}
