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
    ./vendor/vcpkg/bootstrap-vcpkg (.sh or .bat depending on linux or windows)
    ./vendor/vcpkg/vcpkg install

## build 
    cmake --list-presets
    python scripts/build.py (preset)
    
#### alternatively
    cmake --list-presets
    cmake --preset (preset)
    cd build/(preset)
    ninja


