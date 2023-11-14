#!/bin/bash

ROOT_DIR=$(dirname $(readlink -f $0))/../../
echo ${ROOT_DIR}
BUILD_DIR=${ROOT_DIR}/build
VCPKG_DIR=${ROOT_DIR}/vcpkg

mkdir -p ${BUILD_DIR}/

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1  -B ${BUILD_DIR}/ -S ${ROOT_DIR}/ -DCMAKE_TOOLCHAIN_FILE=${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake -G Ninja

pushd ${BUILD_DIR}/
ninja
popd
