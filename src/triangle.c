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
#define MAX_LAYERS 256
#define ENABLE_VALICATION_LAYERS 1
#define VALIDATION_LAYER_COUNT 1

static const char *validation_layers[VALIDATION_LAYER_COUNT] = {
    "VK_LAYER_KHRONOS_validation"};

GLFWwindow *window;
VkInstance instance = {0};
VkDebugUtilsMessengerEXT debug_messenger = {0};
VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};

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
    printf("validation layers requested, but not available!");
    exit(1);
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
    fprintf(stderr, "%d failed to get number of available extensions!\n",
            result);
    exit(-1);
  }

  // get available extensions
  VkExtensionProperties avail_extensions[MAX_EXTENSIONS];
  result = vkEnumerateInstanceExtensionProperties(NULL, &avail_extension_count,
                                                  avail_extensions);
  if (result != VK_SUCCESS) {
    fprintf(stderr, "%d failed to get available extensions!\n", result);
    exit(-1);
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
    fprintf(stderr, "%d failed to create instance!\n", result);
    exit(-1);
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
    fprintf(stderr, "failed to set up debug messenger!");
    exit(1);
  }
}

void init_vulkan() {
  create_instance();
  setup_debug_messenger();
}

void main_loop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

void cleanup() {
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
