#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <span>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Cleanup.h"

struct VulkanContext;
namespace vkcore {
	struct ShaderInputInfo;
	struct ShaderInfo;
}  // namespace vkcore
class DeletionQueue;

namespace vkcore {
	struct DescriptorBindingInfo {
		uint32_t binding;
		VkDescriptorType type;
		uint32_t count;
	};

	using DescriptorSetLayoutInfo = std::vector<DescriptorBindingInfo>;

	struct ShaderLayoutInfo {
		std::vector<DescriptorSetLayoutInfo> descriptorSetLayoutInfos;
		std::vector<VkPushConstantRange> pushConstantRanges;
	};

	struct ShaderLayout {
		std::vector<VkDescriptorSetLayout> shaderDescriptorLayout{};
		std::vector<VkPushConstantRange> pushConstants{};
	};

	vkcore::ShaderLayout createShaderLayout(
		const VulkanContext& ctx,
		const ShaderInputInfo& inputInfo,
		const VkShaderStageFlags stage,
		DeletionQueue& deletionQueue
	);

	VkDescriptorPool createDescriptorPool(
		const VulkanContext& ctx,
		const std::span<const DescriptorBindingInfo>& descriptorInfos,
		const uint32_t maxSets,
		DeletionQueue& deletionQueue
	);

	VkDescriptorPool createDescriptorPool(
		const VulkanContext& ctx,
		const std::span<const DescriptorSetLayoutInfo>& shaderDescriptorInfo,
		const uint32_t maxSets,
		DeletionQueue& deletionQueue
	);

	std::vector<VkDescriptorSetLayout> createDescriptorSetLayouts(
		const VulkanContext& ctx,
		const std::span<const DescriptorSetLayoutInfo>& shaderDescriptorInfo,
		const VkShaderStageFlags stage,
		DeletionQueue& deletionQueue
	);

	std::vector<VkDescriptorSet> allocateDescriptorSets(
		const VulkanContext& ctx,
		const std::span<const VkDescriptorSetLayout>& descriptorSetLayouts,
		const VkDescriptorPool pool
	);

	VkDescriptorSet allocateDescriptorSets(
		const VulkanContext& ctx,
		const VkDescriptorSetLayout descriptorSetLayout,
		const VkDescriptorPool pool
	);

}  // namespace vkcore
