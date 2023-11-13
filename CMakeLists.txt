cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME CitiesAsEcosystems)
project(PROJECT_NAME)

find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(
	SRC_FILES
	${CMAKE_SOURCE_DIR}/src/Main.cpp
	)

add_executable(PROJECT_NAME ${SRC_FILES})

target_link_libraries(PROJECT_NAME PUBLIC SDL2::SDL2 SDL2::SDL2main glm::glm Vulkan::Vulkan)
