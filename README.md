# cities-as-ecosystems

## prerequisite dependencies
cmake,
vulkan sdk,
ninja

## project libraries
SDL2,
glm,
vulkan sdk

## setup
    git clone git@github.com:burningflemingo/cities-as-ecosystems --triplet=x64-${platform} --recursive
    cd ./vcpkg/
    bootstrap-vcpkg (.sh or .bat depending on linux or windows)
    vcpkg install sdl2[core,vulkan] glm vulkan --recurse
    cd ..

## build 
    mkdir build
    cmake -B ./build/ -S ./ -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake -G Ninja
    cd build
    ninja
    
## alternatively
#### windows
    scripts/windows/setup.bat
    scripts/windows/build.bat
#### linux
    scripts/linux/setup.sh
    scripts/linux/build.sh
