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
VkInstance instance;

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

void createInstance() {
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

  uint32_t glfw_extension_count = 0;
  const char **glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  const char *required_extensions[MAX_EXTENSIONS];
  for (int i = 0; i < glfw_extension_count; i++) {
    required_extensions[i] = glfw_extensions[i];
  }
  required_extensions[glfw_extension_count] =
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
  glfw_extension_count += 1;
  required_extensions[glfw_extension_count] =
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
  glfw_extension_count += 1;

  create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

  create_info.enabledExtensionCount = glfw_extension_count;
  create_info.ppEnabledExtensionNames = required_extensions;

  if (ENABLE_VALICATION_LAYERS) {
    create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
    create_info.ppEnabledLayerNames = validation_layers;
  } else {
    create_info.enabledLayerCount = 0;
  }

  // check available extensions
  VkResult result;
  uint32_t extension_count = 0;
  // get count of available extensions
  result = vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
  if (result != VK_SUCCESS) {
    fprintf(stderr, "%d failed to get number of available extensions!\n",
            result);
    exit(-1);
  }

  // get available extensions
  VkExtensionProperties extensions[MAX_EXTENSIONS];
  result = vkEnumerateInstanceExtensionProperties(NULL, &extension_count,
                                                  extensions);
  if (result != VK_SUCCESS) {
    fprintf(stderr, "%d failed to get available extensions!\n", result);
    exit(-1);
  }
  printf("available extensions %d:\n", extension_count);
  for (int i = 0; i < extension_count; i++) {
    printf("\t%s\n", extensions[i].extensionName);
  }

  // check required extensions
  printf("required extensions %d:\n", glfw_extension_count);
  for (int i = 0; i < glfw_extension_count; i++) {
    const char *required_extension = required_extensions[i];
    int exist = 0;
    for (int j = 0; j < extension_count; j++) {
      if (strcmp(required_extension, extensions[j].extensionName) == 0) {
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

void init_vulkan() { createInstance(); }

void main_loop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

void cleanup() {
  vkDestroyInstance(instance, NULL);
  instance = NULL;
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
