@echo off

set "ROOT_DIR=%~dp0%..\.."
echo %ROOT_DIR%
set "BUILD_DIR=%ROOT_DIR%\build"
set "VCPKG_DIR=%ROOT_DIR%\vcpkg"

mkdir %BUILD_DIR%\

%VCPKG_DIR%\bootstrap-vcpkg.sh
%VCPKG_DIR%\vcpkg.exe install sdl2[core,vulkan] glm vulkan --triplet=x64-windows --recurse
