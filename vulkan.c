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
VkShaderModule vertShaderModule;
VkShaderModule fragShaderModule;
VkImageView *swapChainImageViews;
VkRenderPass renderPass;
VkFramebuffer *framebuffers;
VkPipelineLayout pipelineLayout;
VkPipeline pipeline;
VkCommandPool commandPool;
VkCommandBuffer commandBuffer;
float queuePriority = 1.0f;
uint32_t swapChainImageCount;

VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
VkFence inFlightFence;

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

char *vk_loadFile(char *filename, uint32_t *size) {
  FILE *fp;
  fp = fopen(filename, "rb+");
  if (fp == NULL) {
    printf("Failed to open %s\n", filename);
    exit(1);
  }
  fseek(fp, 0l, SEEK_END);
  *size = (uint32_t)ftell(fp);
  rewind(fp);

  char *content = (char *)malloc((*size) * sizeof(char));
  fread(content, 1, *size, fp);

  fclose(fp);
  return content;
}

VkShaderModule vk_createShaderModule(char *filename) {
  uint32_t shaderSize;
  char *shader = vk_loadFile(filename, &shaderSize);

  VkShaderModuleCreateInfo shaderModuleInfo = {};
  shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderModuleInfo.codeSize = shaderSize;
  shaderModuleInfo.pCode = (const uint32_t *)shader;

  VkShaderModule shaderModule;
  VkResult result =
      vkCreateShaderModule(device, &shaderModuleInfo, NULL, &shaderModule);
  if (result != VK_SUCCESS) {
    printf("Error while creating vk shader module for %s\n", filename);
    exit(1);
  }

  free(shader);
  return shaderModule;
}

void vk_createGraphicsPipeline(void) {
  vertShaderModule = vk_createShaderModule("shaders/triangle-vert.spv");
  fragShaderModule = vk_createShaderModule("shaders/triangle-frag.spv");

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo stageInfos[2] = {vertShaderStageInfo,
                                                   fragShaderStageInfo};

  const VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT,
                                           VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
  dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateInfo.dynamicStateCount = 2;
  dynamicStateInfo.pDynamicStates = dynamicStates;

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = NULL;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = NULL;

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
  inputAssemblyInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainExtent.width;
  viewport.height = (float)swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = (VkOffset2D){0, 0};
  scissor.extent = swapChainExtent;

  VkPipelineViewportStateCreateInfo viewportStateInfo = {};
  viewportStateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportStateInfo.viewportCount = 1;
  viewportStateInfo.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizerInfo = {};
  rasterizerInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizerInfo.depthClampEnable = VK_FALSE;
  rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
  rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizerInfo.lineWidth = 1.0f;
  rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizerInfo.depthBiasEnable = VK_FALSE;
  rasterizerInfo.depthBiasConstantFactor = 0.0f;
  rasterizerInfo.depthBiasClamp = 0.0f;
  rasterizerInfo.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisamplingInfo = {};
  multisamplingInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisamplingInfo.sampleShadingEnable = VK_FALSE;
  multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisamplingInfo.minSampleShading = 1.0f;
  multisamplingInfo.pSampleMask = NULL;
  multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
  multisamplingInfo.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlendingState = {};
  colorBlendingState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendingState.logicOpEnable = VK_FALSE;
  colorBlendingState.logicOp = VK_LOGIC_OP_COPY;
  colorBlendingState.attachmentCount = 1;
  colorBlendingState.pAttachments = &colorBlendAttachment;
  colorBlendingState.blendConstants[0] = 0.0f;
  colorBlendingState.blendConstants[1] = 0.0f;
  colorBlendingState.blendConstants[2] = 0.0f;
  colorBlendingState.blendConstants[3] = 0.0f;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = NULL;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = NULL;

  VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL,
                                           &pipelineLayout);
  if (result != VK_SUCCESS) {
    printf("Failed to create vk pipeline layout\n");
    exit(1);
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = stageInfos;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
  pipelineInfo.pViewportState = &viewportStateInfo;
  pipelineInfo.pRasterizationState = &rasterizerInfo;
  pipelineInfo.pMultisampleState = &multisamplingInfo;
  pipelineInfo.pDepthStencilState = NULL;
  pipelineInfo.pColorBlendState = &colorBlendingState;
  pipelineInfo.pDynamicState = &dynamicStateInfo;

  result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                     NULL, &pipeline);
  if (result != VK_SUCCESS) {
    printf("Failed to create vk graphics pipeline\n");
    exit(1);
  }
}

void vk_createRenderPass(void) {
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  VkResult result =
      vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass);
  if (result != VK_SUCCESS) {
    printf("Failed to create vk render pass\n");
    exit(1);
  }
}

void vk_createFramebuffers(void) {
  framebuffers =
      (VkFramebuffer *)malloc(swapChainImageCount * sizeof(VkFramebuffer));
  size_t i;
  for (i = 0; i < swapChainImageCount; i++) {
    VkImageView attachments[] = {swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    VkResult result =
        vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffers[i]);
    if (result != VK_SUCCESS) {
      printf("Failed to create vk framebuffer at index %zu\n", i);
      exit(1);
    }
  }
}

void vk_createCommandPool(void) {
  uint32_t queueFamilyNumber = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyNumber,
                                           VK_NULL_HANDLE);

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyNumber;

  VkResult result = vkCreateCommandPool(device, &poolInfo, NULL, &commandPool);
  if (result != VK_SUCCESS) {
    printf("Failed to create vk command pool\n");
    exit(1);
  }
}

void vk_createCommandBuffer(void) {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkResult result =
      vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
  if (result != VK_SUCCESS) {
    printf("Failed to allocate vk command buffers\n");
    exit(1);
  }
}

void vk_recordCommandBuffer(VkCommandBuffer commandBuffer,
                            uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = NULL;

  VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
  if (result != VK_SUCCESS) {
    printf("Failed to begin recording vk command buffer\n");
    exit(1);
  }

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = framebuffers[imageIndex];
  renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainExtent.width;
  viewport.height = (float)swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = (VkOffset2D){0, 0};
  scissor.extent = swapChainExtent;

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(commandBuffer);

  result = vkEndCommandBuffer(commandBuffer);
  if (result != VK_SUCCESS) {
    printf("Failed to record vk command buffer\n");
    exit(1);
  }
}

void vk_createSyncObjects(void) {
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (vkCreateSemaphore(device, &semaphoreInfo, NULL,
                        &imageAvailableSemaphore) != VK_SUCCESS ||
      vkCreateSemaphore(device, &semaphoreInfo, NULL,
                        &renderFinishedSemaphore) != VK_SUCCESS ||
      vkCreateFence(device, &fenceInfo, NULL, &inFlightFence) != VK_SUCCESS) {
    printf("Failed to create vk semaphores\n");
    exit(1);
  }
}

void vk_renderFrame(void) {
  vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
  vkResetFences(device, 1, &inFlightFence);
  uint32_t imageIndex;
  vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore,
                        VK_NULL_HANDLE, &imageIndex);

  vkResetCommandBuffer(commandBuffer, 0);
  vk_recordCommandBuffer(commandBuffer, imageIndex);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VkResult result = vkQueueSubmit(queue, 1, &submitInfo, inFlightFence);
  if (result != VK_SUCCESS) {
    printf("Failed to submit draw vk command buffer\n");
    exit(1);
  }

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  /* renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency; */

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = NULL;

  vkQueuePresentKHR(queue, &presentInfo);
}

void vk_init(void) {
  vk_createInstance();
  vk_pickPhysicalDevice();
  vk_createLogicalDevice();
  vk_createSurface();
  vk_createSwapChain();
  vk_createImageViews();
  vk_createRenderPass();
  vk_createGraphicsPipeline();
  vk_createFramebuffers();
  vk_createCommandPool();
  vk_createCommandBuffer();
  vk_createSyncObjects();
}

void vk_cleanup(void) {
  vkDestroySemaphore(device, imageAvailableSemaphore, NULL);
  vkDestroySemaphore(device, renderFinishedSemaphore, NULL);
  vkDestroyFence(device, inFlightFence, NULL);
  vkDestroyCommandPool(device, commandPool, NULL);
  size_t i;
  for (i = 0; i < swapChainImageCount; i++) {
    vkDestroyFramebuffer(device, framebuffers[i], NULL);
  }
  vkDestroyPipeline(device, pipeline, NULL);
  vkDestroyPipelineLayout(device, pipelineLayout, NULL);
  vkDestroyRenderPass(device, renderPass, NULL);
  vkDestroyShaderModule(device, fragShaderModule, NULL);
  vkDestroyShaderModule(device, vertShaderModule, NULL);
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
    glfwPollEvents();
    loop();
  }
  vk_cleanup();
  glfw_destroyWindow();
}

void mainLoop(void) {
  /* Main render loop */
  vk_renderFrame();
}

int main(int argc, char **argv) {
  runApp(mainLoop);
  return 0;
}
