cmake_minimum_required(VERSION 3.8)

project(CitiesAsEcosystems)

find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)

set(CMAKE_CXX_STANDARD 20)

set(
	SRC_FILES
	${CMAKE_SOURCE_DIR}/src/Main.cpp
	)

add_executable(CitiesAsEcosystems ${SRC_FILES})

target_link_libraries(CitiesAsEcosystems PUBLIC SDL2::SDL2 SDL2::SDL2main Vulkan::Vulkan)
