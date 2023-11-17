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
    git clone git@github.com:burningflemingo/cities-as-ecosystems --recursive
    cd cities-as-ecosystems
    python scripts/vcpkgInstall.py
    
#### alternatively
    git clone git@github.com:burningflemingo/cities-as-ecosystems --recursive
    cd cities-as-ecosystems
    cd ./vcpkg/
    bootstrap-vcpkg (.sh or .bat depending on linux or windows)
    vcpkg install sdl2[core,vulkan] glm vulkan --recurse
    cd ..

## build 
    python scripts/build.py
    
#### alternatively
    mkdir build
    cmake -B ./build/ -S ./ -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake -G Ninja
    cd build
    ninja


