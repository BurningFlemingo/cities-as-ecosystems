#define GLM_FORCE_DEPTH_ZERO_TO_ONE // opengl depth is -1:1, vulkan is 0:1, this makes it 0:1
#define GLM_ENABLE_EXPERIMENTAL
// #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <stdint.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
