# Vulkan tutorial

Vulkan tutorial with C on MacOS

## Install dependencies

* glfw - windows management
* cglm - C version GLM
* molten-vk - vulkan implementation on MacOS
* vulkan-headers - vulkan headers file

MacOS commands:

``` bash
# Install
brew install glfw cglm molten-vk vulkan-headers

# Environment variables
export VK_LAYER_PATH=/opt/homebrew/opt/vulkan-validationlayers/share/vulkan/explicit_layer.d
export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:/opt/homebrew/opt/vulkan-validationlayers/lib/"

# Build
./build.sh triangle

# build and run
./build.sh triangle --run
```
