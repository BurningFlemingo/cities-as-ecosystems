#!/bin/bash

ROOT_DIR=$(dirname $(readlink -f $0))/../..
BUILD_DIR=${ROOT_DIR}/build
VCPKG_DIR=${ROOT_DIR}/vcpkg
SRC_DIR=${ROOT_DIR}/src

mkdir -p ${BUILD_DIR}/
mkdir -p ${BUILD_DIR}/shaders

glslc ${SRC_DIR}/shaders/first.vert -o ${BUILD_DIR}/shaders/first_vert.spv
glslc ${SRC_DIR}/shaders/first.frag -o ${BUILD_DIR}/shaders/first_frag.spv

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1  -B ${BUILD_DIR}/ -S ${ROOT_DIR}/ -DCMAKE_TOOLCHAIN_FILE=${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake -G Ninja

pushd ${BUILD_DIR}/
ninja
popd
