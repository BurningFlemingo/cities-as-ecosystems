#!/bin/bash

SRC_DIR=..
BUILD_DIR=${SRC_DIR}/build

mkdir -p ${BUILD_DIR}

pushd ${SRC_DIR}/vcpkg/
./bootstrap-vcpkg.sh
./vcpkg install sdl2[core,vulkan] glm vulkan --recurse
popd
