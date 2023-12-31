#include "RendererPCH.h"

#include "Extensions.h"
#include "debug/Debug.h"

#include <iostream>
#include <string>
#include <sstream>
#include <assert.h>
#include <vector>

std::vector<const char*> findExtensions(
		const std::vector<VkExtensionProperties>& avaliableExtensionProperties,
		const std::vector<const char*>& extensionsToFind);

std::vector<const char*> getSurfaceExtensions(SDL_Window* window) {
	uint32_t SDLExtensionCount{};
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, nullptr);

	std::vector<const char*> SDLExtensions(SDLExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, SDLExtensions.data());

	std::vector<const char*> surfaceExtensions;
	surfaceExtensions.reserve(SDLExtensionCount);
	for (const auto& SDLExtension : SDLExtensions) {
		surfaceExtensions.emplace_back(SDLExtension);
	}

	return surfaceExtensions;
}

std::vector<const char*> queryInstanceExtensions(const std::vector<const char*>& extensions) {
	uint32_t avaliableExtensionsCount{};
	vkEnumerateInstanceExtensionProperties(nullptr, &avaliableExtensionsCount, nullptr);

	std::vector<VkExtensionProperties> avaliableExtensions(avaliableExtensionsCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &avaliableExtensionsCount, avaliableExtensions.data());

	std::vector<const char*> foundExtensions{findExtensions(avaliableExtensions, extensions)};
	return foundExtensions;
}

std::vector<const char*> queryInstanceExtensions(
		const std::vector<const char*>& requiredSurfaceExtensions,
		const std::vector<const char*>& additionalRequiredExtensions,
		const std::vector<const char*>& optionalExtensions
) {

	const size_t totalNumExtensions{
		requiredSurfaceExtensions.size() + additionalRequiredExtensions.size() + optionalExtensions.size()
	};

	auto queriedExtensions = [&]() {
		std::vector<const char*> extensions{};
		extensions.reserve(totalNumExtensions);

		extensions.insert(std::end(extensions), std::begin(requiredSurfaceExtensions), std::end(requiredSurfaceExtensions));
		extensions.insert(std::end(extensions), std::begin(additionalRequiredExtensions), std::end(additionalRequiredExtensions));
		extensions.insert(std::end(extensions), std::begin(optionalExtensions), std::end(optionalExtensions));

		return extensions;
	};

	std::vector<const char*> enabledExtensions{queryInstanceExtensions(queriedExtensions())};
	return enabledExtensions;
}

 std::vector<const char*> queryDeviceExtensions(VkPhysicalDevice pDevice, const std::vector<const char*>& extensions) {
	uint32_t avaliableExtensionsCount{};
	vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &avaliableExtensionsCount, nullptr);

	std::vector<VkExtensionProperties> avaliableExtensions(avaliableExtensionsCount);
	vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &avaliableExtensionsCount, avaliableExtensions.data());
	
	std::vector<const char*> foundExtensions{findExtensions(avaliableExtensions, extensions)};
	return foundExtensions;
}

std::vector<const char*> findExtensions(
		 const std::vector<VkExtensionProperties>& avaliableExtensionProperties,
		 const std::vector<const char*>& extensionsToFind) {
	std::stringstream err;
	std::vector<const char*> foundExtensions{};
	for (const auto& extension : extensionsToFind) {
		auto itt{
			std::find_if(
					avaliableExtensionProperties.begin(),
					avaliableExtensionProperties.end(),
					[&](const auto& avaliableExtensionProperty) {
						return strcmp(avaliableExtensionProperty.extensionName, extension) == 0;
					}
			)
		};

		if (itt != avaliableExtensionProperties.end()) {
			foundExtensions.emplace_back(extension);
		} else {
			err << extension << " ";
		}
	}
	assertInfo(err.str().size() == 0, "could not find all instance extensions: ", err.str());

	return foundExtensions;
}
