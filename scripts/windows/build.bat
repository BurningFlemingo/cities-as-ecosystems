@echo off

set "ROOT_DIR=%~dp0%..\.."
set "BUILD_DIR=%ROOT_DIR%\build"
set "VCPKG_DIR=%ROOT_DIR%\vcpkg"

mkdir %BUILD_DIR%\

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1  -B %BUILD_DIR%\ -S %ROOT_DIR%\ -DCMAKE_TOOLCHAIN_FILE=%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake -G Ninja

pushd %BUILD_DIR%\ 
echo %BUILD_DIR%
ninja
popd
