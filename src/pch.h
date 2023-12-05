#define GLM_FORCE_DEPTH_ZERO_TO_ONE // opengl depth is -1:1, vulkan is 0:1, this makes it 0:1
// #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <stdint.h>
#include <vector>
#include <algorithm>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <stdexcept>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
