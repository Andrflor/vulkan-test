#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

GLFWwindow *window;
VkInstance instance;

void initVK(void) {
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Vulkan";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = 0;

  VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
  if (result != VK_SUCCESS) {
    printf("Failed to create vulkan instance\n");
    exit(1);
  }
}

void createWindow(void) {

  if (!glfwInit()) {
    printf("Failed to initialize GLFW\n");
    exit(1);
  }

  window = glfwCreateWindow(800, 600, "Hello Vulkan", NULL, NULL);
  if (!window) {
    printf("Failed to create GLFW window\n");
    glfwTerminate();
    exit(1);
  }

  glfwMakeContextCurrent(window);
  initVK();
}

void destroyWindow(void) {
  vkDestroyInstance(instance, NULL);
  glfwTerminate();
}

void runApp(void loop(void)) {
  createWindow();
  while (!glfwWindowShouldClose(window)) {
    glfwSwapBuffers(window);
    loop();
    glfwPollEvents();
  }
  destroyWindow();
}

void mainLoop(void) {}

int main(int argc, char **argv) {
  runApp(mainLoop);
  return 0;
}
