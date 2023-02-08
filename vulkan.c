#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

/* Here are some other callbacks
glfwSetDropCallback - triggers when a file or text is dropped onto the window.
glfwSetCursorEnterCallback - triggers when the mouse enters or leaves the
window. glfwSetScrollCallback - triggers when the mouse scroll wheel is moved.
glfwSetCharCallback - triggers when a Unicode character is input.
glfwSetCharModsCallback - triggers when a Unicode character is input with a
specific modifier. glfwSetJoystickCallback - triggers when a joystick is
connected or disconnected */

/* TODO: figure out how to properly set VK to resize and draw black */

GLFWwindow *window;
VkInstance instance;
VkInstanceCreateInfo createInfo = {};
VkSurfaceKHR surface;

void initVK(void) {
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Vulkan";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  uint32_t count;
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.ppEnabledExtensionNames =
      glfwGetRequiredInstanceExtensions(&count);
  createInfo.enabledExtensionCount = count;

  VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
  if (result != VK_SUCCESS) {
    printf("Failed to create vulkan instance\n");
    exit(1);
  }

  result = glfwCreateWindowSurface(instance, window, NULL, &surface);
  if (result != VK_SUCCESS) {
    printf("Failed to create vulkan surface\n");
    exit(1);
  }
}

void window_size_callback(GLFWwindow *window, int width, int height) {

  // Get the current extent of the surface
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface,
                                            &capabilities);

  VkExtent2D extent = capabilities.currentExtent;
  extent.width = width;
  extent.height = height;

  // Update the extent of the surface
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkCreateSwapchainKHR(window, &createInfo, NULL, &surface);

  printf("Window resized to %dx%d\n", width, height);
}

void window_focus_callback(GLFWwindow *window, int focused) {
  if (focused) {
    printf("Window is in focus\n");
  } else {
    printf("Window is not in focus\n");
  }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  printf("Key : %s\n", glfwGetKeyName(key, scancode));
}

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  if (action == GLFW_PRESS) {
    printf("Mouse button %d was clicked\n", button);
  }
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
  printf("Mouse is at position %f, %f\n", xpos, ypos);
}

void initWindowCallbacks(void) {
  glfwMakeContextCurrent(window);
  glfwSetWindowSizeCallback(window, window_size_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetWindowFocusCallback(window, window_focus_callback);
}

void createWindow(void) {
  if (!glfwInit()) {
    printf("Failed to initialize GLFW\n");
    exit(1);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(800, 600, "Hello Vulkan", NULL, NULL);
  if (!window) {
    printf("Failed to create GLFW window\n");
    glfwTerminate();
    exit(1);
  }

  initWindowCallbacks();
  initVK();
}

void destroyWindow(void) {
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);
  glfwDestroyWindow(window);
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
