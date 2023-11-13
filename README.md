# cities-as-ecosystems

## prerequisite dependencies
cmake,
vulkan sdk,
ninja

## project dependencies
SDL2,
glm

## setup
    git clone git@github.com:burningflemingo/cities-as-ecosystems --recursive
    cd ./vcpkg/
    bootstrap-vcpkg (.sh or .bat depending on linux or windows)
    vcpkg install sdl2[core,vulkan] glm vulkan --recurse
    cd ..

## build 
    mkdir build
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -B ./build/ -S ./ -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake -G ninja
