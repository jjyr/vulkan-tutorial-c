# Vulkan tutorial

Vulkan tutorial with C on MacOS

## Install dependencies

* glfw - windows management
* cglm - C version GLM
* molten-vk - vulkan implementation on MacOS
* vulkan-headers - vulkan headers file

MacOS commands:

``` bash
# Install dependencies
brew install glfw cglm molten-vk vulkan-headers

# Export environment variables
export VK_LAYER_PATH="/opt/homebrew/opt/vulkan-validationlayers/share/vulkan/explicit_layer.d:VK_LAYER_PATH=/opt/homebrew/opt/vulkan-profiles/share/vulkan/explicit_layer.d"
export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:/opt/homebrew/lib/:/opt/homebrew/opt/vulkan-validationlayers/lib/"
# Enable debug output
export VK_LOADER_DEBUG=all

# Build all
./build.sh

# Build
./build.sh triangle

# Build and run
./build.sh triangle --run
```
