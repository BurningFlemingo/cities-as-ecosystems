// #pragma once
// 
// #include <vulkan/vulkan.h>
// 
// // forward declerations
// struct Device;
// 
// namespace vkutils {
// 	void createBuffer(
// 	    const Device& device,
// 	    const VkDeviceSize size,
// 	    const VkBufferUsageFlags usage,
// 	    const VkMemoryPropertyFlags memFlags, 
// 		const VkSharingMode sharingMode,
// 		VkBuffer* buffer,
// 	    VkDeviceMemory* bufferMemory
// 	);
// 	
// 	void copyBuffers(
// 		const Device& device,
// 		const VkCommandPool transferCmdPool,
// 		const VkQueue transferQueue,
// 		const VkBuffer srcBuffer,
// 		const VkBuffer dstBuffer,
// 		const VkDeviceSize copyAmmount
// 	);
// 
// }
