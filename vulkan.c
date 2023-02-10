#include <stdint.h>
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
VkPhysicalDevice physicalDevice;
VkQueue queue;
VkDevice device;
VkSurfaceKHR surface;
VkSwapchainKHR swapChain;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;
VkImage *swapChainImages;
VkImageView *swapChainImageViews;
VkRenderPass renderPass;
VkFramebuffer framebuffer;
VkPipeline pipeline;
VkCommandBuffer commandBuffer;
float queuePriority = 1.0f;
uint32_t swapChainImageCount;

void vk_pickPhysicalDevice(void) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

  if (deviceCount == 0) {
    printf("Failed to retreive any compatible device\n");
    exit(1);
  }
  VkPhysicalDevice physicalDevices[deviceCount];
  vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices);

  /* TODO: make a scoring for each GPU and pick the best
  This should be done by checking memory/integrated/queues

  int i;
  for (i = 0; i < deviceCount; i++) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevices[i], &deviceFeatures);
  }
  */

  physicalDevice = physicalDevices[0];
}

void vk_createLogicalDevice(void) {
  uint32_t queueFamilyNumber = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyNumber,
                                           VK_NULL_HANDLE);

  VkPhysicalDeviceFeatures deviceFeatures = {};

  VkDeviceQueueCreateInfo queueInfo = {};
  queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueInfo.queueFamilyIndex = queueFamilyNumber;
  queueInfo.queueCount = 1;
  queueInfo.pQueuePriorities = &queuePriority;

  const char extensionList[][VK_MAX_EXTENSION_NAME_SIZE] = {"VK_KHR_swapchain"};
  const char *extensions[] = {extensionList[0]};

  VkDeviceCreateInfo deviceInfo = {};
  deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceInfo.pQueueCreateInfos = &queueInfo;
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.enabledExtensionCount = 1;
  deviceInfo.ppEnabledExtensionNames = extensions;
  deviceInfo.pEnabledFeatures = &deviceFeatures;

  VkResult result = vkCreateDevice(physicalDevice, &deviceInfo, NULL, &device);
  if (result != VK_SUCCESS) {
    printf("Failed to create logical device\n");
    exit(1);
  }

  vkGetDeviceQueue(device, queueFamilyNumber, 0, &queue);
}

void vk_createInstance(void) {
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Vulkan";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo instanceInfo = {};

  uint32_t count;
  instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceInfo.pApplicationInfo = &appInfo;
  instanceInfo.ppEnabledExtensionNames =
      glfwGetRequiredInstanceExtensions(&count);
  instanceInfo.enabledExtensionCount = count;

  VkResult result = vkCreateInstance(&instanceInfo, NULL, &instance);
  if (result != VK_SUCCESS) {
    printf("Failed to create vulkan instance\n");
    exit(1);
  }
}

void vk_createSurface(void) {
  VkResult result = glfwCreateWindowSurface(instance, window, NULL, &surface);
  if (result != VK_SUCCESS) {
    printf("Failed to create vulkan surface\n");
    exit(1);
  }
}

void vk_createSwapChain(void) {
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                            &surfaceCapabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       NULL);
  VkSurfaceFormatKHR *surfaceFormats =
      (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       surfaceFormats);

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                            &presentModeCount, NULL);
  VkPresentModeKHR *presentModes =
      (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                            &presentModeCount, presentModes);

  VkExtent2D extent = surfaceCapabilities.currentExtent;

  int imageCount = surfaceCapabilities.minImageCount;
  if (surfaceCapabilities.maxImageCount == 0 ||
      imageCount + 1 <= surfaceCapabilities.maxImageCount) {
    imageCount++;
  }

  VkSwapchainCreateInfoKHR swapChainInfo = {};
  swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapChainInfo.surface = surface;
  swapChainInfo.minImageCount = imageCount;
  swapChainInfo.imageFormat = surfaceFormats->format;
  swapChainInfo.imageColorSpace = surfaceFormats->colorSpace;
  swapChainInfo.imageExtent = extent;
  swapChainInfo.imageArrayLayers = 1;
  /* TODO: maybe use cocurrent with double qeues ?? */
  swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapChainInfo.preTransform = surfaceCapabilities.currentTransform;
  swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapChainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swapChainInfo.clipped = VK_TRUE;
  swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

  VkResult result =
      vkCreateSwapchainKHR(device, &swapChainInfo, NULL, &swapChain);
  if (result != VK_SUCCESS) {
    printf("Failed to create vk swap chain\n");
    exit(1);
  }

  vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, NULL);
  swapChainImages = (VkImage *)malloc(swapChainImageCount * sizeof(VkImage));

  vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount,
                          swapChainImages);

  swapChainImageFormat = surfaceFormats->format;
  swapChainExtent = extent;

  free(surfaceFormats);
  free(presentModes);
}

void vk_createImageViews(void) {
  swapChainImageViews =
      (VkImageView *)malloc(swapChainImageCount * sizeof(VkImageView));

  size_t i;
  for (i = 0; i < swapChainImageCount; i++) {
    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.image = swapChainImages[i];
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = swapChainImageFormat;
    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    VkResult result = vkCreateImageView(device, &imageViewInfo, NULL,
                                        &swapChainImageViews[i]);
    if (result != VK_SUCCESS) {
      printf("Failed to create vk image view at index: %zu\n", i);
      exit(1);
    }
  }
}

void vk_init(void) {
  vk_createInstance();
  vk_pickPhysicalDevice();
  vk_createLogicalDevice();
  vk_createSurface();
  vk_createSwapChain();
  vk_createImageViews();
}

void vk_cleanup(void) {
  size_t i;
  for (i = 0; i < swapChainImageCount; i++) {
    vkDestroyImageView(device, swapChainImageViews[i], NULL);
  }
  vkDestroySwapchainKHR(device, swapChain, NULL);
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyDevice(device, NULL);
  vkDestroyInstance(instance, NULL);
}

void glfw_callbacks_size(GLFWwindow *window, int width, int height) {
  printf("Window resized to %dx%d\n", width, height);
}

void glfw_callbacks_focus(GLFWwindow *window, int focused) {
  if (focused) {
    printf("Window is in focus\n");
  } else {
    printf("Window is not in focus\n");
  }
}

void glfw_callbacks_key(GLFWwindow *window, int key, int scancode, int action,
                        int mods) {
  printf("Key : %s\n", glfwGetKeyName(key, scancode));
}

void glfw_callbacks_mouseButton(GLFWwindow *window, int button, int action,
                                int mods) {
  if (action == GLFW_PRESS) {
    printf("Mouse button %d was clicked\n", button);
  }
}

void glfw_callbacks_cursorPosition(GLFWwindow *window, double xpos,
                                   double ypos) {
  printf("Mouse is at position %f, %f\n", xpos, ypos);
}

void glfw_callbacks_init(void) {
  glfwMakeContextCurrent(window);
  glfwSetWindowSizeCallback(window, glfw_callbacks_size);
  glfwSetKeyCallback(window, glfw_callbacks_key);
  glfwSetMouseButtonCallback(window, glfw_callbacks_mouseButton);
  glfwSetCursorPosCallback(window, glfw_callbacks_cursorPosition);
  glfwSetWindowFocusCallback(window, glfw_callbacks_focus);
}

void glfw_createWindow(void) {
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

  glfw_callbacks_init();
}

void glfw_destroyWindow(void) {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void runApp(void loop(void)) {
  glfw_createWindow();
  vk_init();
  while (!glfwWindowShouldClose(window)) {
    loop();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  vk_cleanup();
  glfw_destroyWindow();
}

void mainLoop(void) { /* Here should be running the main app logic */
}

int main(int argc, char **argv) {
  runApp(mainLoop);
  return 0;
}
