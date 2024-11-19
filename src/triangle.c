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
#define MAX_SHADER_CODE 4096
#define MAX_FRAMES_IN_FLIGHT 2

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
uint32_t swap_chain_image_views_count = 0;
VkImageView swap_chain_image_views[MAX_SWAP_CHAIN_IMAGES_COUNT];
uint32_t vert_code_len = 0;
char vert_shader_code[MAX_SHADER_CODE];
uint32_t frag_code_len = 0;
char frag_shader_code[MAX_SHADER_CODE];
VkRenderPass render_pass;
VkPipelineLayout pipeline_layout;
VkPipeline graphics_pipeline;
uint32_t swap_chain_framebuffers_count = 0;
VkFramebuffer swap_chain_framebuffers[MAX_SWAP_CHAIN_IMAGES_COUNT];
VkCommandPool command_pool;
VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
uint32_t current_frame = 0;
int framebuffer_resized = 0;

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

static void framebuffer_resize_callback(GLFWwindow *window, int width,
                                        int height) {
  framebuffer_resized = 1;
}

void init_window() {
  if (glfwInit() != GLFW_TRUE) {
    THROW("failed to init glfw\n");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
  glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
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

void create_image_views() {
  swap_chain_image_views_count = swap_chain_images_count;
  for (int i = 0; i < swap_chain_images_count; i++) {
    VkImageViewCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = swap_chain_images[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = swap_chain_image_format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &create_info, NULL,
                          &swap_chain_image_views[i]) != VK_SUCCESS) {
      THROW("failed to create image views!\n");
    }
  }
}

uint32_t read_file(char *filename, char *buffer) {
  FILE *f = fopen(filename, "rb");
  fseek(f, 0, SEEK_END);
  uint32_t filelen = ftell(f);
  rewind(f);
  fread(buffer, filelen, 1, f);
  fclose(f);
  return filelen;
}

VkShaderModule create_shader_module(char *code, uint32_t size) {
  VkShaderModuleCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = size;
  create_info.pCode = (const uint32_t *)code;
  VkShaderModule shader_module;
  if (vkCreateShaderModule(device, &create_info, NULL, &shader_module) !=
      VK_SUCCESS) {
    THROW("failed to create shader module!\n");
  }
  return shader_module;
}

void create_graphics_pipeline() {
  vert_code_len = read_file("shaders/vert.spv", vert_shader_code);
  frag_code_len = read_file("shaders/frag.spv", frag_shader_code);

  VkShaderModule vert_shader_module =
      create_shader_module(vert_shader_code, vert_code_len);
  VkShaderModule frag_shader_module =
      create_shader_module(frag_shader_code, frag_code_len);

  VkPipelineShaderStageCreateInfo vert_shader_stage_info = {0};
  vert_shader_stage_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_stage_info.module = vert_shader_module;
  vert_shader_stage_info.pName = "main";

  VkPipelineShaderStageCreateInfo frag_shader_stage_info = {0};
  frag_shader_stage_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_stage_info.module = frag_shader_module;
  frag_shader_stage_info.pName = "main";

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info,
                                                     frag_shader_stage_info};

  VkDynamicState dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo dynamic_state = {0};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = 2;
  dynamic_state.pDynamicStates = dynamic_states;

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
  vertex_input_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = 0;
  vertex_input_info.pVertexBindingDescriptions = NULL;
  vertex_input_info.vertexAttributeDescriptionCount = 0;
  vertex_input_info.pVertexAttributeDescriptions = NULL;

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {0};
  input_assembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swap_chain_extent.width;
  viewport.height = (float)swap_chain_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {0};
  scissor.extent = swap_chain_extent;

  VkPipelineViewportStateCreateInfo viewport_state = {0};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer = {0};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampling = {0};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = NULL;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
  color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo color_blending = {0};
  color_blending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 0;
  pipeline_layout_info.pSetLayouts = NULL;
  pipeline_layout_info.pushConstantRangeCount = 0;
  pipeline_layout_info.pPushConstantRanges = NULL;

  if (vkCreatePipelineLayout(device, &pipeline_layout_info, NULL,
                             &pipeline_layout) != VK_SUCCESS) {
    THROW("failed to create pipeline layout!\n");
  }

  VkGraphicsPipelineCreateInfo pipeline_info = {0};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pDepthStencilState = NULL;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.pDynamicState = &dynamic_state;
  pipeline_info.layout = pipeline_layout;
  pipeline_info.renderPass = render_pass;
  pipeline_info.subpass = 0;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_info.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL,
                                &graphics_pipeline) != VK_SUCCESS) {
    THROW("failed to create graphics pipeline!\n");
  }

  vkDestroyShaderModule(device, frag_shader_module, NULL);
  vkDestroyShaderModule(device, vert_shader_module, NULL);
}

void create_render_pass() {
  VkAttachmentDescription color_attachment = {0};
  color_attachment.format = swap_chain_image_format;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref = {0};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {0};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;

  VkSubpassDependency dependency = {0};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_info = {0};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &color_attachment;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &render_pass_info, NULL, &render_pass) !=
      VK_SUCCESS) {
    THROW("failed to create render pass!\n");
  }
}

void create_framebuffers() {
  swap_chain_framebuffers_count = swap_chain_image_views_count;
  for (int i = 0; i < swap_chain_image_views_count; i++) {
    VkImageView attachments[] = {swap_chain_image_views[i]};

    VkFramebufferCreateInfo framebuffer_info = {0};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = swap_chain_extent.width;
    framebuffer_info.height = swap_chain_extent.height;
    framebuffer_info.layers = 1;

    if (vkCreateFramebuffer(device, &framebuffer_info, NULL,
                            &swap_chain_framebuffers[i]) != VK_SUCCESS) {
      THROW("failed to create framebuffer!\n");
    }
  }
}

void create_command_pool() {
  QueueFamilyIndices queue_family_indices =
      find_queue_families(physical_device);
  VkCommandPoolCreateInfo pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = queue_family_indices.graphics_family;
  if (vkCreateCommandPool(device, &pool_info, NULL, &command_pool) !=
      VK_SUCCESS) {
    THROW("failed to create command pool!\n");
  }
}

void record_command_buffer(VkCommandBuffer command_buffer,
                           uint32_t image_index) {
  VkCommandBufferBeginInfo begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = 0;
  begin_info.pInheritanceInfo = NULL;

  if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
    THROW("failed to begin recording command buffer!\n");
  }

  VkRenderPassBeginInfo render_pass_info = {0};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = render_pass;
  render_pass_info.framebuffer = swap_chain_framebuffers[image_index];
  render_pass_info.renderArea.extent = swap_chain_extent;

  VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues = &clear_color;

  vkCmdBeginRenderPass(command_buffer, &render_pass_info,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphics_pipeline);

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swap_chain_extent.width;
  viewport.height = (float)swap_chain_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor = {0};
  scissor.extent = swap_chain_extent;
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);

  vkCmdDraw(command_buffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(command_buffer);
  if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
    THROW("failed to record command buffer!\n");
  }
}

void create_command_buffers() {
  VkCommandBufferAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
  if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers) !=
      VK_SUCCESS) {
    THROW("failed to allocate command buffers!");
  }
}

void create_sync_objects() {
  VkSemaphoreCreateInfo semaphore_info = {0};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info = {0};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device, &semaphore_info, NULL,
                          &image_available_semaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphore_info, NULL,
                          &render_finished_semaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fence_info, NULL, &in_flight_fences[i]) !=
            VK_SUCCESS) {
      THROW("failed to create semaphores!\n");
    }
  }
}

void cleanup_swap_chain() {
  for (int i = 0; i < swap_chain_framebuffers_count; i++) {
    vkDestroyFramebuffer(device, swap_chain_framebuffers[i], NULL);
  }

  for (int i = 0; i < swap_chain_images_count; i++) {
    vkDestroyImageView(device, swap_chain_image_views[i], NULL);
  }

  vkDestroySwapchainKHR(device, swap_chain, NULL);
}

void recreate_swap_chain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device);

  cleanup_swap_chain();

  create_swap_chain();
  create_image_views();
  create_framebuffers();
}

void init_vulkan() {
  create_instance();
  setup_debug_messenger();
  create_surface();
  pick_physical_device();
  create_logical_device();
  create_swap_chain();
  create_image_views();
  create_render_pass();
  create_graphics_pipeline();
  create_framebuffers();
  create_command_pool();
  create_command_buffers();
  create_sync_objects();
}

void draw_frame() {
  vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE,
                  UINT64_MAX);

  uint32_t image_index;
  VkResult result = vkAcquireNextImageKHR(
      device, swap_chain, UINT64_MAX, image_available_semaphores[current_frame],
      VK_NULL_HANDLE, &image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreate_swap_chain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    THROW("failed to acquire swap chain image!\n");
  }

  vkResetFences(device, 1, &in_flight_fences[current_frame]);

  vkResetCommandBuffer(command_buffers[current_frame], 0);
  record_command_buffer(command_buffers[current_frame], image_index);

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[] = {image_available_semaphores[current_frame]};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffers[current_frame];

  VkSemaphore signal_semaphores[] = {render_finished_semaphores[current_frame]};
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;

  if (vkQueueSubmit(graphics_queue, 1, &submit_info,
                    in_flight_fences[current_frame]) != VK_SUCCESS) {
    THROW("failed to submit draw command buffer!\n");
  }

  VkPresentInfoKHR present_info = {0};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;

  VkSwapchainKHR swap_chains[] = {swap_chain};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swap_chains;
  present_info.pImageIndices = &image_index;
  present_info.pResults = NULL;
  result = vkQueuePresentKHR(present_queue, &present_info);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      framebuffer_resized) {
    framebuffer_resized = 0;
    recreate_swap_chain();
  } else if (result != VK_SUCCESS) {
    THROW("failed to present swap chain image!\n");
  }
  current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void main_loop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    draw_frame();
  }
  vkDeviceWaitIdle(device);
}

void cleanup() {
  cleanup_swap_chain();

  vkDestroyPipeline(device, graphics_pipeline, NULL);
  vkDestroyPipelineLayout(device, pipeline_layout, NULL);

  vkDestroyRenderPass(device, render_pass, NULL);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, image_available_semaphores[i], NULL);
    vkDestroySemaphore(device, render_finished_semaphores[i], NULL);
    vkDestroyFence(device, in_flight_fences[i], NULL);
  }

  vkDestroyCommandPool(device, command_pool, NULL);

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
