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
#define THROW(...)                                                             \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    exit(1);                                                                   \
  } while (0)

static const char *validation_layers[VALIDATION_LAYER_COUNT] = {
    "VK_LAYER_KHRONOS_validation"};

GLFWwindow *window;
VkInstance instance = {0};
VkDebugUtilsMessengerEXT debug_messenger = {0};
VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;
VkQueue graphics_queue;

typedef struct {
  uint32_t graphics_family;
} QueueFamilyIndices;

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
               void *pUserData) {

  fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);
  return VK_FALSE;
}

void init_window() {
  glfwInit();

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

void get_required_extensions(uint32_t *count, const char **extentions) {
  uint32_t glfw_extension_count = 0;
  const char **glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  for (int i = 0; i < glfw_extension_count; i++) {
    extentions[i] = glfw_extensions[i];
  }
  *count = glfw_extension_count;

  extentions[*count] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
  *count += 1;
  extentions[*count] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
  *count += 1;

  if (ENABLE_VALICATION_LAYERS) {
    extentions[*count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
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

int find_queue_families(VkPhysicalDevice device, QueueFamilyIndices *indices) {
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
  VkQueueFamilyProperties queue_families[64];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           queue_families);

  int complete = 0;
  for (int i = 0; i < queue_family_count; i++) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices->graphics_family = i;
      complete = 1;
    }
  }

  return complete;
}

int is_device_suitable(VkPhysicalDevice device) {
  QueueFamilyIndices indices;
  int found = find_queue_families(device, &indices);
  return found;
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
  QueueFamilyIndices indices;
  if (!find_queue_families(physical_device, &indices)) {
    THROW("Can't find a queue family");
  }

  VkDeviceQueueCreateInfo queue_create_info = {0};
  queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_info.queueFamilyIndex = indices.graphics_family;
  queue_create_info.queueCount = 1;

  float queue_priority = 1.0f;
  queue_create_info.pQueuePriorities = &queue_priority;

  VkPhysicalDeviceFeatures device_features = {0};
  const char *extensions[] = {"VK_KHR_portability_subset"};
  VkDeviceCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pQueueCreateInfos = &queue_create_info;
  create_info.queueCreateInfoCount = 1;
  create_info.pEnabledFeatures = &device_features;
  create_info.enabledExtensionCount = 1;
  create_info.ppEnabledExtensionNames = extensions;
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
}

void init_vulkan() {
  create_instance();
  setup_debug_messenger();
  pick_physical_device();
  create_logical_device();
}

void main_loop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

void cleanup() {
  vkDestroyDevice(device, NULL);
  if (ENABLE_VALICATION_LAYERS) {
    destroy_debug_utils_messenger_ext(instance, debug_messenger, NULL);
  }
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
