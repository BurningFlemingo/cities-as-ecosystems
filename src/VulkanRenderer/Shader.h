#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <filesystem>
#include <span>
#include <unordered_map>

#include "vkcore/ShaderLayout.h"

// forward declerations
class DeletionQueue;
struct VulkanContext;

namespace vkcore {
	struct ShaderInputInfo {
		vkcore::ShaderLayoutInfo layoutInfo;
	};

	struct ShaderSourceInfo {
		std::vector<char> spv;
		VkShaderStageFlags stage;
	};

	struct ShaderInfo {
		ShaderSourceInfo sourceInfo;
		ShaderInputInfo inputInfo;
	};

	struct SharedShaderObjects {
		VkDescriptorPool descriptorPool;
		vkcore::ShaderLayout layout;
	};

	struct UniqueShaderObjects {
		std::vector<VkDescriptorSet> descriptorSets;
		ShaderSourceInfo sourceInfo;
	};

	SharedShaderObjects createSharedShaderObjects(
		const VulkanContext& ctx,
		const ShaderInputInfo& inputInfo,
		const VkShaderStageFlags stage,
		DeletionQueue& deletionQueue
	);

	UniqueShaderObjects createUniqueShaderObjects(
		const VulkanContext& ctx,
		const ShaderSourceInfo& sourceInfo,
		const SharedShaderObjects sharedShaderObjects
	);

	std::vector<ShaderInfo>
		parseShaders(const std::span<const std::filesystem::path>& shaderPaths);

	std::vector<VkPipelineShaderStageCreateInfo> createShaderStages(
		const VulkanContext& ctx,
		const std::span<const ShaderSourceInfo>& shaderSourceInfos,
		DeletionQueue& deletionQueue
	);
}  // namespace vkcore
