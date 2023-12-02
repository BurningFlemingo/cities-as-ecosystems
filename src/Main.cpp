#define STB_IMAGE_IMPLEMENTATION
#include "pch.h"

#include <iostream>

#include <stb/stb_image.h>

#include "debug/Debug.h"

#include "Swapchain.h"
#include "Device.h"
#include "Extensions.h"
#include "debug/Debug.h"
#include "Instance.h"

#include <optional>
#include <fstream>
#include <chrono>
#include <limits.h>
#include <assert.h>

struct ImageInfo {
	int width{};
	int height{};
	int channels{};
	
	uint32_t bytesPerPixel{};

	stbi_uc* pixels{};
};

std::vector<char> readFile(const std::string& filename);

void createBuffer(
    const Device& device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memFlags,
    VkBuffer* buffer,
    VkDeviceMemory* memory
);

void copyBuffers(const Device& device, VkCommandPool transferCmdPool, VkQueue transferQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);

VkCommandBuffer beginTransientCommands(const Device& device, VkCommandPool commandPool);
void endTransientCommands(const Device& device, VkCommandPool pool, VkCommandBuffer cmdBuffer, VkQueue queue);

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
void freeImage(ImageInfo& imageInfo);

struct Vertex {
	glm::vec2 pos;
	glm::vec3 col;
	glm::vec2 texCoord;
};

struct UBO {
	glm::vec2 omo;
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

int main(int argc, char* argv[]) {
	uint32_t WINDOW_HEIGHT{1080 / 2};
	uint32_t WINDOW_WIDTH{1920 / 2};

	if (SDL_Init(SDL_INIT_EVENTS) != 0) {
		std::cerr << "could not init sdl: " << SDL_GetError() << std::endl;
	}

	SDL_Window* window{SDL_CreateWindow("vulkan :D", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE)};
	if (!window) {
		std::cerr << "window could not be created: " << SDL_GetError() << std::endl;
	}

	Instance instance{createInstance(window)};

	VkSurfaceKHR surface{};
	SDL_Vulkan_CreateSurface(window, instance.handle, &surface);

	Device device{createDevice(instance, surface)};

	VkQueue graphicsQueue{};
	VkQueue presentQueue{};
	VkQueue transferQueue{};
	for (auto indexMap : device.queueFamilyIndices.uniqueIndices) {
		for (int i{}; i < indexMap.second.size(); i++) {
			QueueFamily family{indexMap.second[i]};
			uint32_t queueFamilyIndex{indexMap.first};

			switch (family) {
				case QueueFamily::graphics:
					vkGetDeviceQueue(device.logical, queueFamilyIndex, i, &graphicsQueue);
				case QueueFamily::presentation:
					presentQueue = graphicsQueue;
					break;
				case QueueFamily::transfer:
					vkGetDeviceQueue(device.logical, queueFamilyIndex, i, &transferQueue);
					break;
				default:
					break;
			}
		}
	}

	VkCommandPool transferPool{};
	{
		VkCommandPoolCreateInfo transferPoolCreateInfo{};
		transferPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		transferPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		transferPoolCreateInfo.queueFamilyIndex = device.queueFamilyIndices.transferIndex;

		if (vkCreateCommandPool(device.logical, &transferPoolCreateInfo, nullptr, &transferPool) != VK_SUCCESS) {
			std::cerr << "could not create transfer command pool" << std::endl;
		}
	}


	Swapchain swapchain{
		createSwapchain(device.logical, window, surface, device.surfaceSupportDetails)
	};

	VkShaderModule vertexShaderModule{};
	VkShaderModule fragmentShaderModule{};
	{
		std::vector<char> vertShader{readFile("./shaders/first.vert.spv")};
		std::vector<char> fragShader{readFile("./shaders/first.frag.spv")};

		VkShaderModuleCreateInfo vShaderModuleCreateInfo{};
		vShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vShaderModuleCreateInfo.codeSize = vertShader.size();  // size in bytes
		vShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertShader.data());

		VkShaderModuleCreateInfo fShaderModuleCreateInfo{};
		fShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		fShaderModuleCreateInfo.codeSize = fragShader.size();
		fShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragShader.data());

		vkCreateShaderModule(device.logical, &vShaderModuleCreateInfo, nullptr, &vertexShaderModule);
		vkCreateShaderModule(device.logical, &fShaderModuleCreateInfo, nullptr, &fragmentShaderModule);
	}

	VkPipelineShaderStageCreateInfo pipelineStages[2];
	{
		VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
		vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageCreateInfo.module = vertexShaderModule;
		vertShaderStageCreateInfo.pName = "main";
		vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
		fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageCreateInfo.module = fragmentShaderModule;
		fragShaderStageCreateInfo.pName = "main";
		fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		pipelineStages[0] = vertShaderStageCreateInfo;
		pipelineStages[1] = fragShaderStageCreateInfo;
	}

	std::vector<Vertex> vertices = {
		{{-0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
		{{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, 
		{{0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}
	};

	std::vector<uint16_t> indices = {
		0, 1, 2, 1, 3, 2
	};

	VkVertexInputBindingDescription vertexBindingDescription{};
	std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions{};
	{
		vertexBindingDescription.binding = 0;
		vertexBindingDescription.stride = sizeof(Vertex);
		vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		vertexAttributeDescriptions[0].binding = 0;
		vertexAttributeDescriptions[0].offset = offsetof(Vertex, pos);
		vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributeDescriptions[0].location = 0;

		vertexAttributeDescriptions[1].binding = 0;
		vertexAttributeDescriptions[1].offset = offsetof(Vertex, col);
		vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributeDescriptions[1].location = 1;

		vertexAttributeDescriptions[2].binding = 0;
		vertexAttributeDescriptions[2].offset = offsetof(Vertex, texCoord);
		vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributeDescriptions[2].location = 2;
	}

	VkBuffer vertexBuffer{};
	VkDeviceMemory vertexBufferMemory{};
	{
		VkDeviceMemory stagingBufferMemory{};
		VkBuffer stagingBuffer{};

		VkDeviceSize bufferSize{sizeof(vertices[0]) * vertices.size()};

		createBuffer(
				device,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stagingBuffer, &stagingBufferMemory
			);

		void* data;
		vkMapMemory(device.logical, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), bufferSize);
		vkUnmapMemory(device.logical, stagingBufferMemory);

		createBuffer(
				device,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&vertexBuffer, &vertexBufferMemory
			);

		copyBuffers(device, transferPool, transferQueue, stagingBuffer, vertexBuffer, bufferSize);
		
		vkDestroyBuffer(device.logical, stagingBuffer, nullptr);
		vkFreeMemory(device.logical, stagingBufferMemory, nullptr);
	}

	VkBuffer indexBuffer{};
	VkDeviceMemory indexBufferMemory{};
	{
		VkDeviceMemory stagingBufferMemory{};
		VkBuffer stagingBuffer{};

		VkDeviceSize bufferSize{sizeof(indices[0]) * indices.size()};

		createBuffer(
				device,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stagingBuffer, &stagingBufferMemory
			);

		void* data;
		vkMapMemory(device.logical, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), bufferSize);
		vkUnmapMemory(device.logical, stagingBufferMemory);

		createBuffer(
				device,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&indexBuffer, &indexBufferMemory
			);

		copyBuffers(device, transferPool, transferQueue, stagingBuffer, indexBuffer, bufferSize);
		
		vkDestroyBuffer(device.logical, stagingBuffer, nullptr);
		vkFreeMemory(device.logical, stagingBufferMemory, nullptr);
	}

	VkCommandPool commandPool{};
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo{};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = device.queueFamilyIndices.graphicsIndex;

		if (vkCreateCommandPool(device.logical, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
			std::cerr << "could not create command pool" << std::endl;
		}
	}

	VkImage textureImage{};
	VkDeviceMemory textureMemory{};
	createTextureImage(
			device,
			transferPool,
			commandPool,
			transferQueue,
			graphicsQueue,
			"./assets/duck.png",
			&textureImage, 
			&textureMemory
		);
	VkImageView textureImageView{createImageView(device, textureImage, VK_FORMAT_R8G8B8A8_SRGB)};
	VkSampler textureSampler{createSampler(device)};


	uint32_t MAX_FRAMES_IN_FLIGHT{2};

	VkDescriptorPool descriptorPool{};
	{
		constexpr uint32_t poolSizesCount{2};
		VkDescriptorPoolSize poolSizes[poolSizesCount]{};

		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

		VkDescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
		poolCreateInfo.poolSizeCount = poolSizesCount;
		poolCreateInfo.pPoolSizes = poolSizes;

		if (vkCreateDescriptorPool(device.logical, &poolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			std::cerr << "could not create descriptor pool" << std::endl;
		}
	}

	VkDescriptorSetLayout descriptorSetLayout{};
	{
		VkDescriptorSetLayoutBinding uboBinding{};
		uboBinding.descriptorCount = 1;
		uboBinding.binding = 0;
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorSetLayoutBinding samplerBinding{};
		samplerBinding.descriptorCount = 1;
		samplerBinding.binding = 1;
		samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings{uboBinding, samplerBinding};

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.bindingCount = bindings.size();
		layoutCreateInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device.logical, &layoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			std::cerr << "could not create descriptor set layout" << std::endl;
		}
	}

	std::vector<VkDescriptorSet> uboDescriptorSets(MAX_FRAMES_IN_FLIGHT);
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = layouts.size();
		allocInfo.pSetLayouts = layouts.data();
		allocInfo.descriptorPool = descriptorPool;

		if (vkAllocateDescriptorSets(device.logical, &allocInfo, uboDescriptorSets.data()) != VK_SUCCESS) {
			std::cerr << "could not allocate descriptor sets" << std::endl;
		}
	}

	std::vector<VkBuffer> uboBuffers(MAX_FRAMES_IN_FLIGHT);
	std::vector<VkDeviceMemory> uboMemory(MAX_FRAMES_IN_FLIGHT);
	std::vector<void*> uboMappedMem(MAX_FRAMES_IN_FLIGHT);
	{
		VkDeviceSize bufferSize{sizeof(UBO)};

		for (int i{}; i < MAX_FRAMES_IN_FLIGHT; i++) {
			createBuffer(
					device,
					bufferSize,
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					&uboBuffers[i],
					&uboMemory[i]
				);

			vkMapMemory(device.logical, uboMemory[i], 0, bufferSize, 0, &uboMappedMem[i]);
		}
	}

	for (int i{}; i < uboDescriptorSets.size(); i++) {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = textureSampler;
		imageInfo.imageView = textureImageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.range = VK_WHOLE_SIZE;
		bufferInfo.buffer = uboBuffers[i];

		std::array<VkWriteDescriptorSet, 2> writeSets{};
		writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeSets[0].descriptorCount = 1;
		writeSets[0].dstSet = uboDescriptorSets[i];
		writeSets[0].dstBinding = 0;
		writeSets[0].pBufferInfo = &bufferInfo;

		writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeSets[1].descriptorCount = 1;
		writeSets[1].dstSet = uboDescriptorSets[i];
		writeSets[1].dstBinding = 1;
		writeSets[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device.logical, writeSets.size(), writeSets.data(), 0, nullptr);
	}

	VkPipelineLayout pipelineLayout{};
	VkRenderPass renderPass{};
	VkPipeline pipeline{};
	{
		std::vector<VkDynamicState> dynamicStates{
			VK_DYNAMIC_STATE_VIEWPORT, 
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
		dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();

		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
		vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{}; 
		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyCreateInfo.primitiveRestartEnable = 0;

		VkPipelineViewportStateCreateInfo viewportCreateInfo{};
		viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportCreateInfo.scissorCount = 1;
		viewportCreateInfo.viewportCount = 1;
		// viewportCreateInfo.pViewports = &viewport;
		// viewportCreateInfo.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizerCreationInfo{};
		rasterizerCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerCreationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerCreationInfo.depthBiasEnable = VK_FALSE;
		rasterizerCreationInfo.rasterizerDiscardEnable = VK_FALSE;

		rasterizerCreationInfo.lineWidth = 1.0;

		rasterizerCreationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerCreationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

		rasterizerCreationInfo.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{};
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo;
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};
		colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendCreateInfo.attachmentCount = 1;
		colorBlendCreateInfo.pAttachments = &colorBlendAttachment;

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

		if (vkCreatePipelineLayout(device.logical, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			std::cerr << "could not create pipeline layout" << std::endl;
		}

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapchain.format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;

		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassCreateInfo{};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &colorAttachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device.logical, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS) {
			std::cerr << "could not create render pass" << std::endl;
		}

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = pipelineStages;
		pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
		pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		pipelineCreateInfo.pViewportState = &viewportCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterizerCreationInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
		pipelineCreateInfo.pDepthStencilState = nullptr;
		pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;

		pipelineCreateInfo.layout = pipelineLayout;

		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.subpass = 0;

		pipelineCreateInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
			std::cerr << "pipeline could not be created" << std::endl;
		}

		vkDestroyShaderModule(device.logical, vertexShaderModule, nullptr);
		vkDestroyShaderModule(device.logical, fragmentShaderModule, nullptr);
	}

	std::vector<VkImageView> swapchainImageViews{
		createSwapchainImageViews(
				device.logical,
				swapchain)
	};

	std::vector<VkFramebuffer> swapchainFramebuffers{
		createSwapchainFramebuffers(device.logical, swapchainImageViews, swapchain.extent, renderPass)
	};


	std::vector<VkCommandBuffer> commandBuffers(MAX_FRAMES_IN_FLIGHT);
	{
		VkCommandBufferAllocateInfo commandBufferAllocInfo{};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandPool = commandPool;
		commandBufferAllocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		if (vkAllocateCommandBuffers(device.logical, &commandBufferAllocInfo, commandBuffers.data()) != VK_SUCCESS) {
			std::cerr << "could not create command buffers" << std::endl;
		}
	}

	std::vector<VkSemaphore> imageAvaliableSemaphores(MAX_FRAMES_IN_FLIGHT);
	std::vector<VkSemaphore> renderFinishedSemaphores(MAX_FRAMES_IN_FLIGHT);
	std::vector<VkFence> inFlightFences(MAX_FRAMES_IN_FLIGHT);
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (int i{}; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(device.logical, &semaphoreCreateInfo, nullptr, &imageAvaliableSemaphores[i]) != VK_SUCCESS) {
				std::cerr << "could not create semaphore" << std::endl;
			}
			if (vkCreateSemaphore(device.logical, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
				std::cerr << "could not create semaphore" << std::endl;
			}
			if (vkCreateFence(device.logical, &fenceCreateInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				std::cerr << "could not create fence" << std::endl;
			}
		}
	}

	SDL_Event e;
	bool running{true};
	bool windowResized{};
	bool rendering{true};
	uint32_t frame{};

	uint32_t framesRendered{};
	auto startTime{ std::chrono::high_resolution_clock::now() };
	auto fpsTime{startTime};

	while (running) {
			while (SDL_PollEvent(&e)) {
				switch (e.type) {
					case SDL_QUIT:
						running = false;
						break;
					case SDL_KEYDOWN:
						if (e.key.keysym.scancode == SDL_SCANCODE_TAB) {
							running = false;
						}
						break;
					case SDL_WINDOWEVENT:
						switch (e.window.event) {
							case SDL_WINDOWEVENT_RESIZED:
							case SDL_WINDOWEVENT_SIZE_CHANGED:
								windowResized = true;
								break;
							case SDL_WINDOWEVENT_MINIMIZED:
								rendering = false;
								break;
							case SDL_WINDOWEVENT_RESTORED:
								rendering = true;
								break;
							default: break;
						}
						break;
					default: break;
				}
			}

		if (rendering) {
			uint32_t swapchainImageIndex{};
			{
				vkWaitForFences(device.logical, 1, &inFlightFences[frame], VK_TRUE, UINT64_MAX);

				VkResult result {vkAcquireNextImageKHR(device.logical, swapchain.handle, UINT64_MAX, imageAvaliableSemaphores[frame], VK_NULL_HANDLE, &swapchainImageIndex)};
				bool swapchainIsDirty{result == VK_ERROR_OUT_OF_DATE_KHR || windowResized};

				if (swapchainIsDirty) {
					windowResized = false;
					VkSwapchainKHR oldSwapchainHandle{swapchain.handle};
					swapchain = recreateSwapchain(device.logical, window, surface, device.surfaceSupportDetails, oldSwapchainHandle);

					destroySwapchain(device.logical, oldSwapchainHandle, nullptr, nullptr);
					vkWaitForFences(device.logical, MAX_FRAMES_IN_FLIGHT, inFlightFences.data(), VK_TRUE, UINT64_MAX);

					destroySwapchainImageViews(device.logical, &swapchainImageViews, &swapchainFramebuffers);
					swapchainImageViews = createSwapchainImageViews(device.logical, swapchain);
					swapchainFramebuffers = createSwapchainFramebuffers(device.logical, swapchainImageViews, swapchain.extent, renderPass);

					continue;
				}
				if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
					std::cerr << "could not get image from swapchain" << std::endl;
				}
			}

			vkResetFences(device.logical, 1, &inFlightFences[frame]);
			vkResetCommandBuffer(commandBuffers[frame], 0);
			
			{
				auto currentTime{ std::chrono::high_resolution_clock::now() };
				float time{ std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count() };

				UBO ubo{};
				ubo.model = glm::rotate(glm::mat4(1.f), time * glm::radians(360.f), glm::vec3(1.f, 1.f, 1.f));
				ubo.view = glm::lookAt(glm::vec3(2.f, 0.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
				ubo.proj = glm::perspective(glm::radians(45.f), (float)swapchain.extent.width / swapchain.extent.height, 0.1f, 10.f);

				memcpy(uboMappedMem[frame], &ubo, sizeof(ubo));
			}

			{
				VkCommandBufferBeginInfo cmdBuffBeginInfo{};
				cmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	
				if (vkBeginCommandBuffer(commandBuffers[frame], &cmdBuffBeginInfo) != VK_SUCCESS) {
					std::cerr << "could not begin command buffer recording" << std::endl;
				}
			}
	
			{
				VkClearValue clearColor{};
				clearColor.color = {{0.f, 0.f, 0.f, 1.f}};
	
				VkRenderPassBeginInfo renderPassBeginInfo{};
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.renderPass = renderPass;
				renderPassBeginInfo.framebuffer = swapchainFramebuffers[swapchainImageIndex];
				renderPassBeginInfo.renderArea.extent = swapchain.extent;
	
				renderPassBeginInfo.clearValueCount = 1;
				renderPassBeginInfo.pClearValues = &clearColor;
	
				vkCmdBeginRenderPass(commandBuffers[frame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			}
	
			{
				vkCmdBindPipeline(commandBuffers[frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				VkBuffer vertexBuffers[] = {vertexBuffer};
				VkDeviceSize offsets[] = {0};

				vkCmdBindVertexBuffers(commandBuffers[frame], 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffers[frame], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	
				VkViewport viewport{};

				// flipped y of viewport to make glm work out of the box (opengl has y going up)
				viewport.height = -static_cast<float>(swapchain.extent.height);
				viewport.y = (float)swapchain.extent.height;

				viewport.width = static_cast<float>(swapchain.extent.width);
				viewport.maxDepth = 1.0;
	
				VkRect2D scissor{};
				scissor.extent = swapchain.extent;
	
				vkCmdSetViewport(commandBuffers[frame], 0, 1, &viewport);
				vkCmdSetScissor(commandBuffers[frame], 0, 1, &scissor);

				vkCmdBindDescriptorSets(commandBuffers[frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uboDescriptorSets[frame], 0, nullptr);
			}

			vkCmdDrawIndexed(commandBuffers[frame], indices.size(), 1, 0, 0, 0);

			vkCmdEndRenderPass(commandBuffers[frame]);
			if (vkEndCommandBuffer(commandBuffers[frame]) != VK_SUCCESS) {
				std::cerr << "could not record command buffer" << std::endl;
			}

			{
				VkSemaphore waitSemaphores[] = {imageAvaliableSemaphores[frame]};
				VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[frame]};
				VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pCommandBuffers = &commandBuffers[frame];
				submitInfo.pSignalSemaphores = signalSemaphores;
				submitInfo.pWaitSemaphores = waitSemaphores;
				submitInfo.pWaitDstStageMask = waitStages;

				if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[frame]) != VK_SUCCESS) {
					std::cout << "could not submit draw command buffer" << std::endl;
				}

				VkPresentInfoKHR presentInfo{};
				presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				presentInfo.waitSemaphoreCount = 1;
				presentInfo.pWaitSemaphores = signalSemaphores;

				VkSwapchainKHR swapchains[] = {swapchain.handle};
				presentInfo.swapchainCount = 1;
				presentInfo.pSwapchains = swapchains;
				presentInfo.pImageIndices = &swapchainImageIndex;

				VkResult result{vkQueuePresentKHR(presentQueue, &presentInfo)};
				bool swapchainIsDirty{result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || windowResized};

				if (swapchainIsDirty) {
					windowResized = false;
					VkSwapchainKHR oldSwapchainHandle{swapchain.handle};
					swapchain = recreateSwapchain(device.logical, window, surface, device.surfaceSupportDetails, oldSwapchainHandle);

					vkDeviceWaitIdle(device.logical);
					destroySwapchain(device.logical, oldSwapchainHandle, nullptr, nullptr);

					destroySwapchainImageViews(device.logical, &swapchainImageViews, &swapchainFramebuffers);
					swapchainImageViews = createSwapchainImageViews(device.logical, swapchain);
					swapchainFramebuffers = createSwapchainFramebuffers(device.logical, swapchainImageViews, swapchain.extent, renderPass);

					continue;
				}
				if (result != VK_SUCCESS) {
					std::cerr << "could not present image" << std::endl;
				}
			}

			frame = (frame + 1) % MAX_FRAMES_IN_FLIGHT;
			framesRendered++;

			auto currentTime{ std::chrono::high_resolution_clock::now() };
			float duration{ std::chrono::duration<float, std::chrono::seconds::period>(currentTime - fpsTime).count() };
			if (duration >= 1) {
				fpsTime = currentTime;
				std::cout << "frames renderered: " << framesRendered << " | ms per frame: " << 1.f / framesRendered << std::endl;;
				framesRendered = 0;
			}
		}
	}
	vkDeviceWaitIdle(device.logical);
	for (int i{}; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device.logical, imageAvaliableSemaphores[i], nullptr);
		vkDestroySemaphore(device.logical, renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(device.logical, inFlightFences[i], nullptr);

		vkDestroyBuffer(device.logical, uboBuffers[i], nullptr);
		vkFreeMemory(device.logical, uboMemory[i], nullptr);
	}

	destroySwapchain(device.logical, swapchain.handle, &swapchainImageViews, &swapchainFramebuffers);

	vkDestroyBuffer(device.logical, vertexBuffer, nullptr);
	vkFreeMemory(device.logical, vertexBufferMemory, nullptr);

	vkDestroyBuffer(device.logical, indexBuffer, nullptr);
	vkFreeMemory(device.logical, indexBufferMemory, nullptr);

	vkDestroyImage(device.logical, textureImage, nullptr);
	vkFreeMemory(device.logical, textureMemory, nullptr);

	vkDestroySampler(device.logical, textureSampler, nullptr);
	vkDestroyImageView(device.logical, textureImageView, nullptr);

	vkDestroyCommandPool(device.logical, commandPool, nullptr);
	vkDestroyCommandPool(device.logical, transferPool, nullptr);

	vkDestroyDescriptorPool(device.logical, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device.logical, descriptorSetLayout, nullptr);

	vkDestroyPipeline(device.logical, pipeline, nullptr);

	vkDestroyRenderPass(device.logical, renderPass, nullptr);

	vkDestroyPipelineLayout(device.logical, pipelineLayout, nullptr);

	vkDestroySurfaceKHR(instance.handle, surface, nullptr);

	vkDestroyDevice(device.logical, nullptr);
	DEBUG::destroyDebugMessenger(instance.handle, instance.debugMessenger, nullptr);
	vkDestroyInstance(instance.handle, nullptr);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file) {
		std::cout << "could not open shader" << std::endl;
		return {};
	}

	auto fileLength{static_cast<size_t>(file.tellg())};

	std::vector<char> buf(fileLength);
	file.seekg(0);
	file.read(buf.data(), fileLength);
	file.close();

	return buf;
}

uint32_t findValidMemoryTypeIndex(
		const Device& device,
		uint32_t validTypeFlags,
		VkMemoryPropertyFlags requiredMemProperties
	) {
	for (int i{}; i < device.memProperties.memoryTypeCount; i++) {
		if ((validTypeFlags & (1 << i)) &&  // bit i is set only if pDeviceMemProps.memoryTypes[i] is a valid type
			((device.memProperties.memoryTypes[i].propertyFlags &
			  requiredMemProperties)) == requiredMemProperties
		) {  // having more properties isnt invalid
			return i;
		}
	}
	throw std::runtime_error("could not find valid memory type");
}

void createBuffer(
		const Device& device,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags memFlags,
		VkBuffer* buffer,
		VkDeviceMemory* memory
	) {
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.size = size;

	if (vkCreateBuffer(device.logical, &bufferCreateInfo, nullptr, buffer) != VK_SUCCESS) {
		throw std::runtime_error("could not create buffer");
	}

	VkMemoryRequirements bufferMemoryRequirements{};
	vkGetBufferMemoryRequirements(device.logical, *buffer, &bufferMemoryRequirements);

	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = bufferMemoryRequirements.size;
	memAllocInfo.memoryTypeIndex = findValidMemoryTypeIndex(device, bufferMemoryRequirements.memoryTypeBits, memFlags);

	// TODO: use a better allocation strategy
	if (vkAllocateMemory(device.logical, &memAllocInfo, nullptr, memory) != VK_SUCCESS) {
		throw std::runtime_error("could not allocate buffer");
	}

	vkBindBufferMemory(device.logical, *buffer, *memory, 0);
}

void copyBuffers(const Device& device, VkCommandPool transferCmdPool, VkQueue transferQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize) {
	VkCommandBuffer cmdBuffer{beginTransientCommands(device, transferCmdPool)};

	VkBufferCopy bufCopyRegion{};
	bufCopyRegion.size = bufferSize;

	vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &bufCopyRegion);

	endTransientCommands(device, transferCmdPool, cmdBuffer, transferQueue);
}

VkCommandBuffer beginTransientCommands(const Device& device, VkCommandPool commandPool) {
	VkCommandBuffer commandBuffer;
	{
		VkCommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocInfo.commandPool = commandPool;
		cmdBufferAllocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(device.logical, &cmdBufferAllocInfo, &commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("could not allocate command buffer");
		}
	}

	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo);

	return commandBuffer;
}

void endTransientCommands(const Device& device, VkCommandPool pool, VkCommandBuffer cmdBuffer, VkQueue queue) {
	vkEndCommandBuffer(cmdBuffer);

	VkSubmitInfo bufSubmitInfo{};
	bufSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	bufSubmitInfo.commandBufferCount = 1;
	bufSubmitInfo.pCommandBuffers = &cmdBuffer;

	if (vkQueueSubmit(queue, 1, &bufSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("could not submit command buffer");
	}
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device.logical, pool, 1, &cmdBuffer);
}

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

// image must be in optimal layout
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
		assert("could not create sampler");
	}

	return sampler;
}

ImageInfo loadRGBAImage(const std::string& filePath) {
	ImageInfo iInfo{};
	stbi_uc* pixels{stbi_load(filePath.c_str(), &iInfo.width, &iInfo.height, &iInfo.channels, STBI_rgb_alpha)};
	iInfo.pixels = pixels;

	if (!pixels) {
		assert("could not load image");
	}

	iInfo.bytesPerPixel = sizeof(uint8_t) * 4;

	return iInfo;
}

void freeImage(ImageInfo& imageInfo) {
	stbi_image_free(imageInfo.pixels);
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

		createBuffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

		void* mappedMem{};
		vkMapMemory(device.logical, stagingBufferMemory, 0, imageSize, 0, &mappedMem);
		memcpy(mappedMem, imageInfo.pixels, imageSize);
		vkUnmapMemory(device.logical, stagingBufferMemory);

		freeImage(imageInfo);
		imageInfo.pixels = nullptr;
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
