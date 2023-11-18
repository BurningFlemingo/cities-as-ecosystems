#include <iostream>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <stdint.h>
#include <vector>
#include <algorithm>
#include <optional>
#include <limits.h>
#include <fstream>

constexpr uint32_t WINDOW_HEIGHT{1080 / 2};
constexpr uint32_t WINDOW_WIDTH{1920 / 2};

struct SwapchainSupportDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	std::vector<VkSurfaceFormatKHR> surfaceFormats{};
	std::vector<VkPresentModeKHR> presentModes{};
};

std::vector<char> readFile(const std::string& filename);

std::vector<const char*> getLayers();
std::vector<const char*> getExtensions(SDL_Window* window);
std::vector<const char*> getDeviceExtensions(VkPhysicalDevice pDevice, const std::vector<const char*>& requiredExtensions);
SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice pDevice, VkSurfaceKHR surface);

VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& avaliableFormats);
VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
VkExtent2D chooseSwapchainExtent(SDL_Window* window, VkSurfaceCapabilitiesKHR capabilities);

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

// dll loaded functions
PFN_vkCreateDebugUtilsMessengerEXT load_PFK_vkCreateDebugUtilsMessengerEXT(VkInstance instance);
PFN_vkDestroyDebugUtilsMessengerEXT load_PFK_vkDestroyDebugUtilsMessengerEXT(VkInstance instance);

VkDebugUtilsMessengerCreateInfoEXT populateDebugCreateInfo();

int main(int argc, char* argv[]) {
	if (SDL_Init(SDL_INIT_EVENTS) != 0) {
		std::cerr << "could not init sdl: " << SDL_GetError() << std::endl;
	}

	SDL_Window* window{SDL_CreateWindow("vulkan :D", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN)};
	if (!window) {
		std::cerr << "window could not be created: " << SDL_GetError() << std::endl;
	}

	VkInstance instance;
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "vulkan :D";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "VulkEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		std::vector<const char*> enabledExtensions{getExtensions(window)};
		std::vector<const char*> enabledLayers{getLayers()};

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{populateDebugCreateInfo()};

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = enabledExtensions.size();
		createInfo.ppEnabledExtensionNames = enabledExtensions.data();
		createInfo.enabledLayerCount = enabledLayers.size();
		createInfo.ppEnabledLayerNames = enabledLayers.data();
		createInfo.pNext = &debugCreateInfo;

		if(vkCreateInstance(&createInfo, nullptr, &instance)) {
			std::cerr << "could not create vulkan instance" << std::endl;
		}
	}

	PFN_vkCreateDebugUtilsMessengerEXT VK_CreateDebugUtilsMessengerEXT{load_PFK_vkCreateDebugUtilsMessengerEXT(instance)};
	PFN_vkDestroyDebugUtilsMessengerEXT VK_DestroyDebugUtilsMessengerEXT{load_PFK_vkDestroyDebugUtilsMessengerEXT(instance)};

	VkDebugUtilsMessengerEXT debugMessenger{};
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo{populateDebugCreateInfo()};
		if(VK_CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			std::cerr << "could not create debug messenger" << std::endl;
		}
	}

	VkSurfaceKHR surface{};
	{
		SDL_Vulkan_CreateSurface(window, instance, &surface);
	}

	VkPhysicalDevice physicalDevice{};
	std::optional<uint32_t> graphicsQueueFamilyIndex{};
	std::optional<uint32_t> presentQueueFamilyIndex{};
	std::vector<const char*> deviceExtensions{};
	SwapchainSupportDetails swapchainSupportDetails{};
	{
		std::vector<const char*> requiredDeviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

		uint32_t nPhysicalDevices{};
		vkEnumeratePhysicalDevices(instance, &nPhysicalDevices, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(nPhysicalDevices);
		vkEnumeratePhysicalDevices(instance, &nPhysicalDevices, physicalDevices.data());

		int highestRating{-1};
		for (const auto& pDevice : physicalDevices) {
			VkPhysicalDeviceProperties pDeviceProps{};

			vkGetPhysicalDeviceProperties(pDevice, &pDeviceProps);

			int currentRating{};

			switch(pDeviceProps.deviceType) {
				case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
					currentRating += 1000;
					break;
				case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
					currentRating += 100;
					break;
				case VK_PHYSICAL_DEVICE_TYPE_CPU:
					currentRating += 10;
					break;
				default:
					break;
			}

			uint32_t queueFamilyCount{};
			vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, queueFamilyProps.data());

			std::optional<uint32_t> graphicsFamilyIndex;
			std::optional<uint32_t> presentFamilyIndex;

			for (int i{}; i < queueFamilyProps.size(); i++) {
				if (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					graphicsFamilyIndex = i;
				}

				VkBool32 surfaceSupport{};
				vkGetPhysicalDeviceSurfaceSupportKHR(pDevice, graphicsFamilyIndex.value(), surface, &surfaceSupport);
				if (surfaceSupport) {
					presentFamilyIndex = i;
				}
			}

			if (graphicsFamilyIndex.has_value() && presentFamilyIndex.has_value()) {

				std::vector<const char*> tempDeviceExtensions = getDeviceExtensions(pDevice, requiredDeviceExtensions);
				if (currentRating > highestRating && !tempDeviceExtensions.empty()) {
					SwapchainSupportDetails thisDeviceSwapchainSupport{querySwapchainSupport(pDevice, surface)};
					bool swapchainSupported{
						!thisDeviceSwapchainSupport.surfaceFormats.empty() &&
						!thisDeviceSwapchainSupport.presentModes.empty()
					};
					if (swapchainSupported) {
						chooseSwapchainSurfaceFormat(thisDeviceSwapchainSupport.surfaceFormats);
						graphicsQueueFamilyIndex = graphicsFamilyIndex.value();
						presentQueueFamilyIndex = presentFamilyIndex.value();
						deviceExtensions = tempDeviceExtensions;
						physicalDevice = pDevice;

						swapchainSupportDetails = thisDeviceSwapchainSupport;
					}
				}
			}
		}
	}

	VkDevice device{};
	{
		const float queuePriority{1.f};

		VkDeviceQueueCreateInfo graphicsQueueCreateInfo{};
		graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphicsQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex.value();
		graphicsQueueCreateInfo.queueCount = 1;
		graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;

		VkDeviceQueueCreateInfo presentQueueCreateInfo{};
		presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		presentQueueCreateInfo.queueFamilyIndex = presentQueueFamilyIndex.value();
		presentQueueCreateInfo.queueCount = 1;
		presentQueueCreateInfo.pQueuePriorities = &queuePriority;

		VkDeviceQueueCreateInfo queuesToCreate[2];
		queuesToCreate[0] = graphicsQueueCreateInfo;
		queuesToCreate[1] = presentQueueCreateInfo;

		VkPhysicalDeviceFeatures pDeviceFeats{};
		vkGetPhysicalDeviceFeatures(physicalDevice, &pDeviceFeats);

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = queuesToCreate;
		deviceCreateInfo.queueCreateInfoCount = 2;
		deviceCreateInfo.pEnabledFeatures = &pDeviceFeats;
		deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

		vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
	}

	VkQueue graphicsQueue{};
	vkGetDeviceQueue(device, graphicsQueueFamilyIndex.value(), 0, &graphicsQueue);

	VkQueue presentQueue{};
	vkGetDeviceQueue(device, presentQueueFamilyIndex.value(), 0, &presentQueue);

	VkSurfaceFormatKHR surfaceFormat{chooseSwapchainSurfaceFormat(swapchainSupportDetails.surfaceFormats)};
	VkPresentModeKHR presentMode{chooseSwapchainPresentMode(swapchainSupportDetails.presentModes)};
	VkExtent2D surfaceExtent{chooseSwapchainExtent(window, swapchainSupportDetails.surfaceCapabilities)};

	VkSwapchainKHR swapchain{};
	{
		uint32_t imageCount{};
		imageCount = swapchainSupportDetails.surfaceCapabilities.minImageCount + 1;
		if (imageCount > swapchainSupportDetails.surfaceCapabilities.maxImageCount && swapchainSupportDetails.surfaceCapabilities.maxImageCount > 0) {
			imageCount = swapchainSupportDetails.surfaceCapabilities.maxImageCount;
		}

		uint32_t queueFamilyIndices[2]{ graphicsQueueFamilyIndex.value(), presentQueueFamilyIndex.value() };

		VkSwapchainCreateInfoKHR swapchainCreateInfo{};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainCreateInfo.imageFormat = surfaceFormat.format;
		swapchainCreateInfo.imageExtent = surfaceExtent;
		swapchainCreateInfo.surface = surface;
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.preTransform = swapchainSupportDetails.surfaceCapabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
	}

	uint32_t swapchainImageCount{};
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);

	std::vector<VkImage> swapchainImages(swapchainImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());

	std::vector<VkImageView> imageViews;
	{
		for (const auto& image : swapchainImages) {
			VkImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.image = image;
			imageViewCreateInfo.format = surfaceFormat.format;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			VkImageView imageView{};
			vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView);
			imageViews.emplace_back(imageView);
		}
	}

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
		fShaderModuleCreateInfo.codeSize = vertShader.size();
		fShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertShader.data());

		vkCreateShaderModule(device, &vShaderModuleCreateInfo, nullptr, &vertexShaderModule);
		vkCreateShaderModule(device, &fShaderModuleCreateInfo, nullptr, &fragmentShaderModule);
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

	SDL_Event e;
	bool running{true};
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
				default:
					break;
			}
		}
	}

	for (auto& imageView : imageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}
	vkDestroyShaderModule(device, vertexShaderModule, nullptr);
	vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(device, nullptr);
	VK_DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	vkDestroyInstance(instance, nullptr);
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
	file.read(buf.data(), fileLength);
	file.close();

	return buf;
}

std::vector<const char*> getExtensions(SDL_Window* window) {
	std::vector<const char*> enabledExtensions;

	uint32_t SDLExtensionCount{};
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, nullptr);

	std::vector<const char*> SDLExtensions(SDLExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, SDLExtensions.data());

	std::vector<const char*> extensions;
	extensions.reserve(SDLExtensionCount);
	for (const auto& extension : SDLExtensions) {
		extensions.emplace_back(extension);
	}
	extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

	uint32_t avaliableExtensionCount{};
	vkEnumerateInstanceExtensionProperties(nullptr, &avaliableExtensionCount, nullptr);

	std::vector<VkExtensionProperties> avaliableExtensions(avaliableExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &avaliableExtensionCount, avaliableExtensions.data());

	for (const auto& extension : extensions) {
		auto itt{
			std::find_if(
					avaliableExtensions.begin(),
					avaliableExtensions.end(),
					[&](const auto& avaliableExtension) {return strcmp(avaliableExtension.extensionName, extension) == 0; }
					)
		};

		if (itt != avaliableExtensions.end()) {
			enabledExtensions.emplace_back(extension);
		}
	}
	if (enabledExtensions.size() < extensions.size()) {
		std::cout << "not all extensions avaliable: " << enabledExtensions.size() << " of " << extensions.size() << std::endl;
	}

	return enabledExtensions;
}

 std::vector<const char*> getDeviceExtensions(VkPhysicalDevice pDevice, const std::vector<const char*>& requiredExtensions) {
	uint32_t avaliableExtensionsCount{};
	vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &avaliableExtensionsCount, nullptr);

	std::vector<VkExtensionProperties> avaliableExtensions(avaliableExtensionsCount);
	vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &avaliableExtensionsCount, avaliableExtensions.data());

	std::vector<const char*> extensions{};
	for (const auto& requiredExtension : requiredExtensions) {
		auto itt{
			std::find_if(
					avaliableExtensions.begin(),
					avaliableExtensions.end(),
					[&](const auto& avaliableExtension) {
						return strcmp(avaliableExtension.extensionName, requiredExtension) == 0;
					}
			)
		};

		if (itt != avaliableExtensions.end()) {
			extensions.emplace_back(requiredExtension);
		} else {
			std::cout << requiredExtension << " could not be found" << std::endl;
			return {};
		}
	}

	return extensions;
}

std::vector<const char*> getLayers() {
	std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};

	uint32_t layerCount{};
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> avaliableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, avaliableLayers.data());

	std::vector<const char*> enabledLayers;
	for (const auto& validationLayer : validationLayers) {
		auto itt{
			std::find_if(
					avaliableLayers.begin(),
					avaliableLayers.end(),
					[&](const auto& layer) { return strcmp(validationLayer, layer.layerName) == 0; }
					)
		};

		if (itt != avaliableLayers.end()) {
			enabledLayers.emplace_back(validationLayer);
		}
	}
	if (enabledLayers.size() < validationLayers.size()) {
		std::cout << "not all validation layers avaliable: " << enabledLayers.size() << " of " << validationLayers.size() << std::endl;
	}

	return validationLayers;
}


SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice pDevice, VkSurfaceKHR surface) {
	SwapchainSupportDetails swapchainDetails{};

	uint32_t surfaceFormatsCount{};
	vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, surface, &surfaceFormatsCount, nullptr);

	if (surfaceFormatsCount) {
		swapchainDetails.surfaceFormats.resize(surfaceFormatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, surface, &surfaceFormatsCount, swapchainDetails.surfaceFormats.data());
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pDevice, surface, &swapchainDetails.surfaceCapabilities);

	uint32_t presentModesCount{};
	vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice, surface, &presentModesCount, nullptr);

	if (presentModesCount) {
		swapchainDetails.presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice, surface, &presentModesCount, swapchainDetails.presentModes.data());
	}

	return swapchainDetails;
}

VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& avaliableFormats) {
	VkSurfaceFormatKHR bestFormat{};
	int bestFormatRating{-1};

	for (const auto& format : avaliableFormats) {
		int thisFormatRating{};
		if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			thisFormatRating += 10;
		}
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {
			thisFormatRating += 100;
		}

		if (thisFormatRating > bestFormatRating) {
			bestFormat = format;
			bestFormatRating = thisFormatRating;
		}
	}
	return bestFormat;
}

VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& presentModes) {
	for (const auto& presentMode : presentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapchainExtent(SDL_Window* window, VkSurfaceCapabilitiesKHR capabilities) {
	int width{};	
	int height{};	
	SDL_Vulkan_GetDrawableSize(window, &width, &height);

	VkExtent2D windowExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	if (capabilities.maxImageExtent.width > 0) {
		windowExtent.width = std::clamp(
				windowExtent.width, 
				capabilities.minImageExtent.width,
				capabilities.maxImageExtent.width);
	} else {
		windowExtent.width = std::max(windowExtent.width, 
				capabilities.minImageExtent.width);
	}

	if (capabilities.maxImageExtent.height > 0) {
		windowExtent.height = std::clamp(
				windowExtent.height, 
				capabilities.minImageExtent.height,
				capabilities.maxImageExtent.height);
	} else {
		windowExtent.height = std::max(windowExtent.height, 
				capabilities.minImageExtent.height);
	}

	return windowExtent;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

	std::string severityStr{};
	switch(messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			severityStr = "INFO";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			severityStr = "VERBOSE";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			severityStr = "WARNING";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			severityStr = "ERROR";
			break;
		default:
			break;
	}

    std::cerr << "\tvalidation layer: " << severityStr << " : " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

// create stub
PFN_vkCreateDebugUtilsMessengerEXT load_PFK_vkCreateDebugUtilsMessengerEXT(VkInstance instance) {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    return func;
}

PFN_vkDestroyDebugUtilsMessengerEXT load_PFK_vkDestroyDebugUtilsMessengerEXT(VkInstance instance) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDestroyUtilsMessengerEXT"));
    return func;
}

VkDebugUtilsMessengerCreateInfoEXT populateDebugCreateInfo() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};

	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;

	return createInfo;
}
