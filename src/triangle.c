#include "vulkan/vulkan_core.h"
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#define WIDTH 800
#define HEIGHT 600
#define MAX_EXTENSIONS 256
#define MAX_LAYERS 64
#define MAX_DEVICES 16
#define ENABLE_VALICATION_LAYERS 1
#define VALIDATION_LAYER_COUNT 1
#define DEVICE_EXTENSIONS_COUNT 2
#define QUEUE_COUNT 2
#define MAX_SWAP_CHAIN_IMAGES_COUNT 16

#define THROW(...)                                                             \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    exit(1);                                                                   \
  } while (0)

static const char *validation_layers[VALIDATION_LAYER_COUNT] = {
    "VK_LAYER_KHRONOS_validation"};

static const char *device_extensions[DEVICE_EXTENSIONS_COUNT] = {
    "VK_KHR_portability_subset", VK_KHR_SWAPCHAIN_EXTENSION_NAME};

GLFWwindow *window;
VkInstance instance = {0};
VkDebugUtilsMessengerEXT debug_messenger = {0};
VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;
VkQueue graphics_queue;
VkQueue present_queue;
VkSurfaceKHR surface;
VkSwapchainKHR swap_chain;
uint32_t swap_chain_images_count = 0;
VkImage swap_chain_images[MAX_SWAP_CHAIN_IMAGES_COUNT];
VkFormat swap_chain_image_format;
VkExtent2D swap_chain_extent;

typedef struct {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR formats[256];
  uint32_t formats_len;
  VkPresentModeKHR present_modes[16];
  uint32_t present_modes_len;
} SwapChainSupportDetails;

void query_swap_chain_support(VkPhysicalDevice device,
                              SwapChainSupportDetails *details) {
  // capabilities
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &(details->capabilities));
  // formats
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &(details->formats_len),
                                       NULL);
  if (details->formats_len != 0) {
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, surface, &(details->formats_len), details->formats);
  }

  // present mode
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      device, surface, &(details->present_modes_len), NULL);
  if (details->present_modes_len != 0) {
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &(details->present_modes_len), details->present_modes);
  }
}

typedef struct {
  int32_t graphics_family;
  int32_t present_family;
} QueueFamilyIndices;

int QFI_is_complete(QueueFamilyIndices *indices) {
  return indices->present_family >= 0 && indices->present_family >= 0;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
               void *pUserData) {

  fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);
  return VK_FALSE;
}

void init_window() {
  if (glfwInit() != GLFW_TRUE) {
    THROW("failed to init glfw\n");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
}

int check_validation_layer_support() {
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, NULL);

  VkLayerProperties available_layers[MAX_LAYERS];
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

  for (int i = 0; i < VALIDATION_LAYER_COUNT; i++) {
    int layer_found = 0;

    for (int j = 0; j < layer_count; j++) {
      if (strcmp(validation_layers[i], available_layers[j].layerName) == 0) {
        layer_found = 1;
        break;
      }
    }

    if (!layer_found) {
      return 0;
    }
  }

  return 1;
}

void get_required_extensions(uint32_t *count, const char **extensions) {
  uint32_t glfw_extension_count = 0;
  const char **glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  for (int i = 0; i < glfw_extension_count; i++) {
    extensions[i] = glfw_extensions[i];
  }
  *count = glfw_extension_count;

  extensions[*count] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
  *count += 1;
  extensions[*count] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
  *count += 1;

  if (ENABLE_VALICATION_LAYERS) {
    extensions[*count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    *count += 1;
  }
}

void populate_debug_messenger_create_info(
    VkDebugUtilsMessengerCreateInfoEXT *create_info) {
  create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info->messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info->pfnUserCallback = debug_callback;
}

void create_instance() {
  if (ENABLE_VALICATION_LAYERS && !check_validation_layer_support()) {
    THROW("validation layers requested, but not available!\n");
  }

  VkApplicationInfo app_info = {0};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Hello Triangle";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "No Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

  uint32_t required_extensions_count = 0;
  const char *required_extensions[MAX_EXTENSIONS];
  get_required_extensions(&required_extensions_count, required_extensions);

  create_info.enabledExtensionCount = required_extensions_count;
  create_info.ppEnabledExtensionNames = required_extensions;

  if (ENABLE_VALICATION_LAYERS) {
    create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
    create_info.ppEnabledLayerNames = validation_layers;
    populate_debug_messenger_create_info(&debug_create_info);
    create_info.pNext =
        (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
  } else {
    create_info.enabledLayerCount = 0;
    create_info.pNext = NULL;
  }

  // check available extensions
  VkResult result;
  uint32_t avail_extension_count = 0;
  // get count of available extensions
  result = vkEnumerateInstanceExtensionProperties(NULL, &avail_extension_count,
                                                  NULL);
  if (result != VK_SUCCESS) {
    THROW("%d failed to get number of available extensions!\n", result);
  }

  // get available extensions
  VkExtensionProperties avail_extensions[MAX_EXTENSIONS];
  result = vkEnumerateInstanceExtensionProperties(NULL, &avail_extension_count,
                                                  avail_extensions);
  if (result != VK_SUCCESS) {
    THROW("%d failed to get available extensions!\n", result);
  }
  printf("available extensions %d:\n", avail_extension_count);
  for (int i = 0; i < avail_extension_count; i++) {
    printf("\t%s\n", avail_extensions[i].extensionName);
  }

  // check required extensions
  printf("required extensions %d:\n", required_extensions_count);
  for (int i = 0; i < required_extensions_count; i++) {
    const char *required_extension = required_extensions[i];
    int exist = 0;
    for (int j = 0; j < avail_extension_count; j++) {
      if (strcmp(required_extension, avail_extensions[j].extensionName) == 0) {
        exist = 1;
        break;
      }
    }

    if (exist) {
      printf("\t%s - ok\n", required_extension);
    } else {
      printf("\t%s - fail\n", required_extension);
    }
  }

  result = vkCreateInstance(&create_info, NULL, &instance);
  if (result != VK_SUCCESS) {
    THROW("%d failed to create instance!\n", result);
  }
}

VkResult create_debug_utils_messenger_ext(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != NULL) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void destroy_debug_utils_messenger_ext(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks *pAllocator) {
  PFN_vkDestroyDebugUtilsMessengerEXT func =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != NULL) {
    func(instance, debugMessenger, pAllocator);
  }
}

void setup_debug_messenger() {
  if (!ENABLE_VALICATION_LAYERS)
    return;
  VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
  populate_debug_messenger_create_info(&create_info);

  if (create_debug_utils_messenger_ext(instance, &create_info, NULL,
                                       &debug_messenger) != VK_SUCCESS) {
    THROW("failed to set up debug messenger!\n");
  }
}

QueueFamilyIndices find_queue_families(VkPhysicalDevice device) {
  QueueFamilyIndices indices;
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
  VkQueueFamilyProperties queue_families[64];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           queue_families);

  for (int i = 0; i < queue_family_count; i++) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphics_family = i;
    }
    VkBool32 present_support = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
    if (present_support) {
      indices.present_family = i;
    }
  }

  return indices;
}

int check_device_extension_support(VkPhysicalDevice device) {
  uint32_t extension_count = 0;
  vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);
  VkExtensionProperties available_extensions[256];
  vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count,
                                       available_extensions);

  for (int i = 0; i < DEVICE_EXTENSIONS_COUNT; i++) {
    int found = 0;
    for (int j = 0; j < extension_count; j++) {
      if (strcmp(device_extensions[i], available_extensions[j].extensionName) ==
          0) {
        found = 1;
        break;
      }
    }

    if (!found) {
      return 0;
    }
  }

  return 1;
}

int is_device_suitable(VkPhysicalDevice device) {
  QueueFamilyIndices indices = find_queue_families(device);
  int extensions_supported = check_device_extension_support(device);
  int swap_chain_adequate = 0;
  if (extensions_supported) {
    SwapChainSupportDetails swap_chain_support = {0};
    query_swap_chain_support(device, &swap_chain_support);

    swap_chain_adequate = swap_chain_support.formats_len > 0 &&
                          swap_chain_support.present_modes_len > 0;
  }
  int is_complete = QFI_is_complete(&indices);
  return is_complete && extensions_supported && swap_chain_adequate;
}

VkSurfaceFormatKHR
choose_swap_surface_format(uint32_t count,
                           VkSurfaceFormatKHR available_formats[]) {
  for (int i = 0; i < count; i++) {
    if (available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return available_formats[i];
    }
  }
  return available_formats[0];
}

VkPresentModeKHR
choose_swap_present_mode(uint32_t count,
                         VkPresentModeKHR available_present_modes[]) {
  for (int i = 0; i < count; i++) {
    if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return available_present_modes[i];
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_swap_extent(VkSurfaceCapabilitiesKHR *capabilities) {
  if (capabilities->currentExtent.width != UINT32_MAX) {
    return capabilities->currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D actual_extent = {width, height};
    if (actual_extent.width < capabilities->minImageExtent.width)
      actual_extent.width = capabilities->minImageExtent.width;
    if (actual_extent.width > capabilities->maxImageExtent.width)
      actual_extent.width = capabilities->maxImageExtent.width;
    if (actual_extent.height < capabilities->minImageExtent.height)
      actual_extent.height = capabilities->minImageExtent.height;
    if (actual_extent.height > capabilities->maxImageExtent.height)
      actual_extent.height = capabilities->maxImageExtent.height;

    return actual_extent;
  }
}

void pick_physical_device() {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, NULL);
  if (device_count == 0) {
    THROW("failed to find GPUs with Vulkan support!\n");
  }
  VkPhysicalDevice devices[MAX_DEVICES] = {0};
  vkEnumeratePhysicalDevices(instance, &device_count, devices);

  for (int i = 0; i < device_count; i++) {
    if (is_device_suitable(devices[i])) {
      physical_device = devices[i];
      break;
    }
  }

  if (physical_device == VK_NULL_HANDLE) {
    THROW("failed to find suitable GPU!\n");
  }
}

void create_logical_device() {
  QueueFamilyIndices indices = find_queue_families(physical_device);
  if (!QFI_is_complete(&indices)) {
    THROW("Can't find a queue family");
  }

  uint32_t queue_len = 0;
  VkDeviceQueueCreateInfo queue_create_infos[QUEUE_COUNT] = {0};
  uint32_t unique_queue_families[QUEUE_COUNT] = {indices.graphics_family,
                                                 indices.present_family};

  // push unique queue
  for (int i = 0; i < QUEUE_COUNT; i++) {
    int unique = 1;
    for (int j = 0; j < queue_len; j++) {
      if (unique_queue_families[i] == unique_queue_families[j]) {
        unique = 0;
      }
    }
    if (unique) {
      unique_queue_families[queue_len] = unique_queue_families[i];
      queue_len += 1;
    }
  }

  printf("Found %d unique queue\n", queue_len);

  float queue_priority = 1.0f;

  for (int i = 0; i < queue_len; i++) {
    VkDeviceQueueCreateInfo *queue_create_info = &queue_create_infos[i];
    queue_create_info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info->queueFamilyIndex = unique_queue_families[i];
    queue_create_info->queueCount = 1;
    queue_create_info->pQueuePriorities = &queue_priority;
  }

  VkPhysicalDeviceFeatures device_features = {0};
  VkDeviceCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pQueueCreateInfos = queue_create_infos;
  create_info.queueCreateInfoCount = queue_len;
  create_info.pEnabledFeatures = &device_features;
  create_info.enabledExtensionCount = DEVICE_EXTENSIONS_COUNT;
  create_info.ppEnabledExtensionNames = device_extensions;
  if (ENABLE_VALICATION_LAYERS) {
    create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
    create_info.ppEnabledLayerNames = validation_layers;
  } else {
    create_info.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physical_device, &create_info, NULL, &device) !=
      VK_SUCCESS) {
    THROW("failed to create logical device!\n");
  }

  vkGetDeviceQueue(device, indices.graphics_family, 0, &graphics_queue);
  vkGetDeviceQueue(device, indices.present_family, 0, &present_queue);
}

void create_surface() {
  if (glfwVulkanSupported() != GLFW_TRUE) {
    THROW("glfw not support vulkan\n");
  }
  VkResult r = glfwCreateWindowSurface(instance, window, NULL, &surface);
  if (r != VK_SUCCESS) {
    THROW("failed to create window surface! err: %d\n", r);
  }
}

void create_swap_chain() {
  SwapChainSupportDetails swap_chain_support = {0};
  query_swap_chain_support(physical_device, &swap_chain_support);
  VkSurfaceFormatKHR surface_format = choose_swap_surface_format(
      swap_chain_support.formats_len, swap_chain_support.formats);
  VkPresentModeKHR present_mode = choose_swap_present_mode(
      swap_chain_support.present_modes_len, swap_chain_support.present_modes);
  VkExtent2D extent = choose_swap_extent(&swap_chain_support.capabilities);
  uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
  if (swap_chain_support.capabilities.maxImageCount > 0 &&
      image_count > swap_chain_support.capabilities.maxImageCount) {
    image_count = swap_chain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface;
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  QueueFamilyIndices indices = find_queue_families(physical_device);
  uint32_t queue_family_indices[] = {indices.graphics_family,
                                     indices.present_family};
  if (indices.graphics_family != indices.present_family) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = NULL;
  }

  create_info.preTransform = swap_chain_support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &create_info, NULL, &swap_chain) !=
      VK_SUCCESS) {
    THROW("failed to create swap chain!\n");
  }

  vkGetSwapchainImagesKHR(device, swap_chain, &swap_chain_images_count, NULL);
  vkGetSwapchainImagesKHR(device, swap_chain, &swap_chain_images_count,
                          swap_chain_images);
  swap_chain_image_format = surface_format.format;
  swap_chain_extent = extent;
}

void init_vulkan() {
  create_instance();
  setup_debug_messenger();
  create_surface();
  pick_physical_device();
  create_logical_device();
  create_swap_chain();
}

void main_loop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

void cleanup() {
  vkDestroySwapchainKHR(device, swap_chain, NULL);
  vkDestroyDevice(device, NULL);
  if (ENABLE_VALICATION_LAYERS) {
    destroy_debug_utils_messenger_ext(instance, debug_messenger, NULL);
  }
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);
  glfwDestroyWindow(window);
  glfwTerminate();
}

void run() {
  init_window();
  init_vulkan();
  main_loop();
  cleanup();
}

int main() {
  run();
  return 0;
}
