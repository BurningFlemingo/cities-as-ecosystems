#!/bin/bash

ROOT_DIR=$(dirname $(readlink -f "$0"))../..
BUILD_DIR=${ROOT_DIR}/build
VCPKG_DIR=${ROOT_DIR}/vcpkg

mkdir -p ${BUILD_DIR}/

${VCPKG_DIR}/bootstrap-vcpkg.sh
${VCPKG_DIR}/vcpkg install sdl2[core,vulkan] glm vulkan --triplet=x64-linux --recurse
