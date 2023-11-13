#!/bin/bash

SRC_DIR=..
BUILD_DIR=${SRC_DIR}/build

mkdir -p ${BUILD_DIR}

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1  -B ${BUILD_DIR} -S ${SRC_DIR} -DCMAKE_TOOLCHAIN_FILE=${SRC_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake -G Ninja

pushd ${BUILD_DIR}
ninja
popd
