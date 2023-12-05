#define GLM_ENABLE_EXPERIMENTAL
#include "pch.h"

#include <iostream>

#include "debug/Debug.h"

#include "Swapchain.h"
#include "Device.h"
#include "Extensions.h"
#include "debug/Debug.h"
#include "Instance.h"
#include "Image.h"
#include "Buffer.h"
#include "FileIO.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader/tiny_obj_loader.h"

#include <optional>
#include <fstream>
#include <chrono>
#include <limits.h>
#include <functional>

using namespace VkUtils;

struct Vertex {
	glm::vec3 pos;
	glm::vec3 col;
	glm::vec2 texCoord;

	bool operator==(const Vertex& v) const {
		return v.pos == pos && v.col == col && v.texCoord == texCoord;
	}
};

template <>
struct std::hash<Vertex> {
	size_t operator()(const Vertex& vertex) const {
		return (std::hash<glm::vec3>()(vertex.pos)) ^
		((std::hash<glm::vec2>()(vertex.texCoord) << 1) >> 1) ^
		(std::hash<glm::vec3>()(vertex.col) << 1);
	}
};

struct UBO {
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
	assertFatal(window, "window could not be created: ", SDL_GetError());

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
		createSwapchain(device, window, surface)
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

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	{
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> mats;

		std::string warn, err;
		std::string filename{"assets/models/vikingRoom.obj"};
		tinyobj::attrib_t attrib{};

		if(!tinyobj::LoadObj(&attrib, &shapes, &mats, &warn, &err, filename.c_str())) {
			logWarning("could not load model");
		}

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
					attrib.vertices[3 * index.vertex_index + 0], 
				};
				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0], 
					attrib.texcoords[2 * index.texcoord_index + 1],
				};
				vertex.col = {0.f, 0.f, 0.f};

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = vertices.size();
					vertices.emplace_back(vertex);
				}

				indices.emplace_back(uniqueVertices[vertex]);
			}
		}
	}

	VkBuffer vertexBuffer{};
	VkDeviceMemory vertexBufferMemory{};
	{
		VkDeviceMemory stagingBufferMemory{};
		VkBuffer stagingBuffer{};

		VkDeviceSize bufferSize{sizeof(Vertex) * vertices.size()};

		createBuffer(
				device,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VK_SHARING_MODE_EXCLUSIVE,
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
				VK_SHARING_MODE_EXCLUSIVE,
				&vertexBuffer, &vertexBufferMemory
			);

		copyBuffers(device, transferPool, transferQueue, stagingBuffer, vertexBuffer, bufferSize);
		
		vkDestroyBuffer(device.logical, stagingBuffer, nullptr);
		vkFreeMemory(device.logical, stagingBufferMemory, nullptr);
	}

	VkVertexInputBindingDescription vertexBindingDescription{};
	std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions{};
	{
		vertexBindingDescription.binding = 0;
		vertexBindingDescription.stride = sizeof(Vertex);
		vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		vertexAttributeDescriptions[0].binding = 0;
		vertexAttributeDescriptions[0].offset = offsetof(Vertex, pos);
		vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
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

	Image depthBuffer{};
	createImage(
			device,
			swapchain.extent.width, swapchain.extent.height,
			VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&depthBuffer.handle, 
			&depthBuffer.memory
	);

	depthBuffer.view = createImageView(device, depthBuffer.handle, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);


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
				VK_SHARING_MODE_EXCLUSIVE,
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
				VK_SHARING_MODE_EXCLUSIVE,
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
			"./assets/textures/vikingRoom.png",
			&textureImage, 
			&textureMemory
		);
	VkImageView textureImageView{createImageView(device, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT)};
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
					VK_SHARING_MODE_EXCLUSIVE,
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
		rasterizerCreationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		rasterizerCreationInfo.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{};
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCreateInfo.depthTestEnable = VK_TRUE;
		depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
		depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;

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

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VK_FORMAT_D32_SFLOAT;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency colorDependency{};
		colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		colorDependency.dstSubpass = 0;

		colorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		
		VkSubpassDependency depthDependency{};
		depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		depthDependency.dstSubpass = 0;

		depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkSubpassDependency, 2> dependencies{colorDependency, depthDependency};
		std::array<VkAttachmentDescription, 2> attachments{colorAttachment, depthAttachment};

		VkRenderPassCreateInfo renderPassCreateInfo{};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = attachments.size();
		renderPassCreateInfo.pAttachments = attachments.data();
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = dependencies.size();
		renderPassCreateInfo.pDependencies = dependencies.data();

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
		pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;

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
				device,
				swapchain)
	};

	std::vector<VkFramebuffer> swapchainFramebuffers{
		createSwapchainFramebuffers(device, swapchainImageViews, depthBuffer.view, swapchain, renderPass)
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
					swapchain = recreateSwapchain(device, window, surface, oldSwapchainHandle);

					destroySwapchain(device, oldSwapchainHandle, nullptr, nullptr);
					vkWaitForFences(device.logical, MAX_FRAMES_IN_FLIGHT, inFlightFences.data(), VK_TRUE, UINT64_MAX);

					destroySwapchainImageViews(device, &swapchainImageViews, &swapchainFramebuffers);
					swapchainImageViews = createSwapchainImageViews(device, swapchain);
					swapchainFramebuffers = createSwapchainFramebuffers(device, swapchainImageViews, depthBuffer.view, swapchain, renderPass);

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
				// ubo.model = glm::rotate(glm::mat4(1.f), time * glm::radians(360.f), glm::vec3(1.f, 1.f, 1.f));
				ubo.model = glm::mat4(1.0);
				ubo.view = glm::lookAt(glm::vec3(cos(time) * 2.f, 0.f, sin(time) * 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
				ubo.proj = glm::perspective(glm::radians(45.f), (float)swapchain.extent.width / swapchain.extent.height, 0.1f, 100.f);

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
				std::array<VkClearValue, 2> clearColors{};
				clearColors[0].color = {{0.f, 0.f, 0.f, 1.f}};
				clearColors[1].depthStencil.depth = {1.f};
	
				VkRenderPassBeginInfo renderPassBeginInfo{};
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.renderPass = renderPass;
				renderPassBeginInfo.framebuffer = swapchainFramebuffers[swapchainImageIndex];
				renderPassBeginInfo.renderArea.extent = swapchain.extent;
	
				renderPassBeginInfo.clearValueCount = clearColors.size();
				renderPassBeginInfo.pClearValues = clearColors.data();
	
				vkCmdBeginRenderPass(commandBuffers[frame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			}
	
			{
				vkCmdBindPipeline(commandBuffers[frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				VkBuffer vertexBuffers[] = {vertexBuffer};
				VkDeviceSize offsets[] = {0};

				vkCmdBindVertexBuffers(commandBuffers[frame], 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffers[frame], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	
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
				VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[frame]};
				VkSemaphore waitSemaphores[] = {imageAvaliableSemaphores[frame]};
				VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT};

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
					swapchain = recreateSwapchain(device, window, surface, oldSwapchainHandle);

					vkDeviceWaitIdle(device.logical);
					destroySwapchain(device, oldSwapchainHandle, nullptr, nullptr);

					destroySwapchainImageViews(device, &swapchainImageViews, &swapchainFramebuffers);
					swapchainImageViews = createSwapchainImageViews(device, swapchain);
					swapchainFramebuffers = createSwapchainFramebuffers(device, swapchainImageViews, depthBuffer.view, swapchain, renderPass);

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

	destroySwapchain(device, swapchain.handle, &swapchainImageViews, &swapchainFramebuffers);

	vkDestroyBuffer(device.logical, vertexBuffer, nullptr);
	vkFreeMemory(device.logical, vertexBufferMemory, nullptr);

	vkDestroyBuffer(device.logical, indexBuffer, nullptr);
	vkFreeMemory(device.logical, indexBufferMemory, nullptr);

	vkDestroyImage(device.logical, textureImage, nullptr);
	vkFreeMemory(device.logical, textureMemory, nullptr);

	vkDestroyImage(device.logical, depthBuffer.handle, nullptr);
	vkDestroyImageView(device.logical, depthBuffer.view, nullptr);
	vkFreeMemory(device.logical, depthBuffer.memory, nullptr);

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
