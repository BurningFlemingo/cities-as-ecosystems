cmake_minimum_required(VERSION 3.8)

project(CitiesAsEcosystems)
set(CMAKE_SUPPRESS_REGENERATION true)

find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS glslc)
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

find_program(GLSLC NAMES glslc HINTS Vulkan::glslc)

set(CMAKE_CXX_STANDARD 20)

set(SHADERS_SRC_DIR "${CMAKE_SOURCE_DIR}/src/shaders")
set(SHADERS_BIN_DIR "${CMAKE_BINARY_DIR}/shaders/")

file(GLOB SHADERS
	"${SHADERS_SRC_DIR}/*.vert"
	"${SHADERS_SRC_DIR}/*.frag"
	"${SHADERS_SRC_DIR}/*.comp"
	"${SHADERS_SRC_DIR}/*.geom"
	"${SHADERS_SRC_DIR}/*.tesc"
	"${SHADERS_SRC_DIR}/*.tese")

file(MAKE_DIRECTORY "${SHADERS_BIN_DIR}")

foreach(SHADER ${SHADERS})
	get_filename_component(SHADER_NAME ${SHADER} NAME)
	set(SHADER_BIN_NAME "${SHADERS_BIN_DIR}/${SHADER_NAME}.spv")
	add_custom_command(
		MAIN_DEPENDENCY "${SHADER_BIN_DIR}"
		DEPENDS "${SHADER}"
		OUTPUT "${SHADER_BIN_NAME}"
		COMMAND "${GLSLC}" "${SHADER}" "-o" "${SHADER_BIN_NAME}"
		COMMENT "Compiling ${SHADER_NAME}"
		VERBATIM)
	list(APPEND SPV_SHADERS "${SHADER_BIN_NAME}")
endforeach()

add_custom_target(build_shaders DEPENDS ${SPV_SHADERS})

set (SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set (VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")

set (VULKAN_RENDERER_DIR ${SRC_DIR}/VulkanRenderer)

set(SRC_FILES
	${SRC_DIR}/Main.cpp
	${SRC_DIR}/utils/FileIO.cpp

	${VULKAN_RENDERER_DIR}/Context.cpp
	${VULKAN_RENDERER_DIR}/State.cpp
	${VULKAN_RENDERER_DIR}/Cleanup.cpp
	${VULKAN_RENDERER_DIR}/ImGuiIntegration.cpp

	${VULKAN_RENDERER_DIR}/vkutils/Device.cpp
	${VULKAN_RENDERER_DIR}/vkutils/Commands.cpp
	${VULKAN_RENDERER_DIR}/vkutils/Synchronization.cpp
	${VULKAN_RENDERER_DIR}/vkutils/Memory.cpp

	${VULKAN_RENDERER_DIR}/vkcore/ShaderLayout.cpp

	${VULKAN_RENDERER_DIR}/Renderer.cpp
	${VULKAN_RENDERER_DIR}/Swapchain.cpp
	${VULKAN_RENDERER_DIR}/Device.cpp
	${VULKAN_RENDERER_DIR}/Instance.cpp
	${VULKAN_RENDERER_DIR}/Buffer.cpp
	${VULKAN_RENDERER_DIR}/Image.cpp
	${VULKAN_RENDERER_DIR}/Extensions.cpp
	${VULKAN_RENDERER_DIR}/Pipelines.cpp
	${VULKAN_RENDERER_DIR}/DefaultCreateInfos.cpp

	${VULKAN_RENDERER_DIR}/Shader.cpp

	${VENDOR_DIR}/SingleHeaderImplementations.cpp

	${SRC_DIR}/debug/vulkan/ValidationLayers.cpp
	${SRC_DIR}/CAEngine/CAEngine.cpp
)

add_executable(CitiesAsEcosystems ${SRC_FILES})

target_link_libraries(CitiesAsEcosystems PRIVATE SDL2::SDL2 SDL2::SDL2main Vulkan::Vulkan unofficial::spirv-reflect::spirv-reflect GPUOpen::VulkanMemoryAllocator imgui::imgui)
target_include_directories(CitiesAsEcosystems PRIVATE ${SRC_DIR} "${CMAKE_SOURCE_DIR}/vendor/")

target_precompile_headers(CitiesAsEcosystems PRIVATE ${SRC_DIR}/VulkanRenderer/RendererPCH.h)

if (CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
	target_compile_definitions(CitiesAsEcosystems
		PRIVATE
		DEBUG_VALIDATION_LAYERS
		ENABLE_WARNINGS
		ENABLE_INFO
	)
else()
endif()

if (EXISTS ${CMAKE_BINARY_DIR}/compile_commands.json)
	# file (REMOVE ${CMAKE_SOURCE_DIR}/compile_commands.json)
	# file (COPY ${CMAKE_BINARY_DIR}/compile_commands.json DESTINATION ${CMAKE_SOURCE_DIR}/compile_commands.json)
	add_custom_command(
		TARGET CitiesAsEcosystems POST_BUILD
	    COMMAND ${CMAKE_COMMAND} -E copy
	        "${CMAKE_BINARY_DIR}/compile_commands.json"
	        "${CMAKE_SOURCE_DIR}"
	    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
	    COMMENT "Copying compile_commands.json to src directory"
	    VERBATIM
	)
endif()

set (ASSETS_DIR ${SRC_DIR})

if (EXISTS "${ASSETS_DIR}")
add_custom_command(
	TARGET CitiesAsEcosystems
    COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${ASSETS_DIR}"
		"${CMAKE_BINARY_DIR}/assets"
		DEPENDS "${ASSETS_DIR}"
	DEPENDS ${CMAKE_BINARY_DIR}/assets/
    COMMENT "Copying assets to bin directory"
    VERBATIM
)

endif()

add_dependencies(CitiesAsEcosystems build_shaders)
