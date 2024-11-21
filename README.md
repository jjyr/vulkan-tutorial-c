# Vulkan tutorial

Vulkan tutorial with C on MacOS

## Install dependencies

* glfw - windows management
* cglm - C version GLM
* molten-vk - vulkan implementation on MacOS

MacOS commands:

``` bash
# Install dependencies

# glfw
brew install glfw
# cglm
brew install cglm
# vulkan
brew install molten-vk vulkan-headers vulkan-extensionlayer vulkan-tools vulkan-utility-libraries vulkan-loader vulkan-validationlayers vulkan-profiles vulkan-volk

# install headers
cp deps/*.h /usr/local/include

# Export environment variables
export VK_LAYER_PATH="/opt/homebrew/opt/vulkan-validationlayers/share/vulkan/explicit_layer.d:VK_LAYER_PATH=/opt/homebrew/opt/vulkan-profiles/share/vulkan/explicit_layer.d"
export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:/opt/homebrew/lib/:/opt/homebrew/opt/vulkan-validationlayers/lib/"
# Enable debug output
export VK_LOADER_DEBUG=all

# Compile shaders

./compile-shaders.sh

# Build all
./build.sh

# Build
./build.sh triangle

# Build and run
./build.sh triangle --run
```
