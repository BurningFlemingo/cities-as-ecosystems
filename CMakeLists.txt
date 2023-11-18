cmake_minimum_required(VERSION 3.8)

project(CitiesAsEcosystems)

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS glslc)
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

set(SRC_FILES
	"${CMAKE_SOURCE_DIR}/src/Main.cpp")

add_executable(CitiesAsEcosystems ${SRC_FILES})

target_link_libraries(CitiesAsEcosystems PRIVATE Vulkan::Vulkan SDL2::SDL2 SDL2::SDL2main)

add_custom_command(
	TARGET CitiesAsEcosystems
	POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
	"${CMAKE_BINARY_DIR}/compile_commands.json"
	"${CMAKE_SOURCE_DIR}/src/compile_commands.json"
	DEPENDS "${CMAKE_BINARY_DIR}/compile_commands.json"
	COMMENT "Copying compile_commands.json to src directory"
	VERBATIM)

# currently no assets
# add_custom_command(
# 	TARGET CitiesAsEcosystems
# 	POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy
# 	"${CMAKE_SOURCE_DIR}/src/assets/"
# 	"${CMAKE_BINARY_DIR}/assets/"
# 	DEPENDS "${CMAKE_SOURCE_DIR}/src/assets/"
# 	COMMENT "Copying assets to bin directory"
# 	VERBATIM)

add_dependencies(CitiesAsEcosystems build_shaders)
