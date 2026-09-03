#include "context.h"
#include <cstdint>
#include <print>
#include <src/core/pch.h>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace n_context {
  static PFN_vkCreateDebugUtilsMessengerEXT  pfnCreateDebugUtilsMessengerEXT  = nullptr;
  static PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT = nullptr;

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* pUserData)
  {
    std::print("Validation Layer: {}\n", pCallbackData->pMessage);
    return VK_FALSE;
  }

  // the constructer for the context.
  void createContext(Context &context, Window &window)
  {
    // setting up app name and Vulkan API 1.4.
    VkApplicationInfo appInfo {};
    appInfo.sType             = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName  = "Crucible";
    appInfo.apiVersion        = VK_API_VERSION_1_4;

    uint32_t extensionsCount = 0;
    const char** glfwExts    = glfwGetRequiredInstanceExtensions(&extensionsCount);

    // add debug_utils on top of whatever glfw needs
    std::vector<const char*> extensions(glfwExts, glfwExts + extensionsCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // instance setting up info
    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo         = &appInfo;
    instanceCreateInfo.enabledExtensionCount    = static_cast<uint32_t>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames  = extensions.data();

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &context.m_instance) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to load vk instance\n");
    }

    // resolve the debug_utils functions NOW that we have a valid instance
    pfnCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)
      vkGetInstanceProcAddr(context.m_instance, "vkCreateDebugUtilsMessengerEXT");
    pfnDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
      vkGetInstanceProcAddr(context.m_instance, "vkDestroyDebugUtilsMessengerEXT");

    if (!pfnCreateDebugUtilsMessengerEXT || !pfnDestroyDebugUtilsMessengerEXT)
      throw std::runtime_error("VK_EXT_debug_utils not available");

    // create debug messenger info
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;

    if (pfnCreateDebugUtilsMessengerEXT(context.m_instance, &createInfo, nullptr, &context.m_debugMessenger) != VK_SUCCESS) {
      throw std::runtime_error("Failed to load messenger");
    }

    // Physical Device
    uint32_t deviceCount{};
    vkEnumeratePhysicalDevices(context.m_instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(context.m_instance, &deviceCount, devices.data());

    // index through all.
    uint32_t deviceIndex{};
    VkPhysicalDeviceProperties2 deviceProperties {};
    deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

    // printing all devices.
    vkGetPhysicalDeviceProperties2(devices[deviceIndex], &deviceProperties);
    std::print("GPU: {}\n", deviceProperties.properties.deviceName);

    context.m_physicalDevice = devices[deviceIndex];

    // set amount of queue family.
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context.m_physicalDevice, &queueFamilyCount, nullptr);

    // storing in a vector.
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(context.m_physicalDevice, &queueFamilyCount, queueFamilies.data());

    // going through all families with VK_QUEUE_GRAPHICS_BIT.
    uint32_t queueFamily = 0;
    for (size_t i = 0; i < queueFamilies.size(); i++)
    {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        queueFamily = i;
        break;
      }
    }

    context.m_queueFamily = queueFamily;

    const float priorities = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo {};
    queueCreateInfo.sType             = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex  = queueFamily;
    queueCreateInfo.queueCount        = 1;
    queueCreateInfo.pQueuePriorities  = &priorities; 

    const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkPhysicalDeviceVulkan12Features enabledVk12Features {};
    enabledVk12Features.sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    enabledVk12Features.pNext               = nullptr;
    enabledVk12Features.descriptorIndexing  = VK_TRUE;
    enabledVk12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    enabledVk12Features.descriptorBindingVariableDescriptorCount  = VK_TRUE;
    enabledVk12Features.runtimeDescriptorArray  = VK_TRUE;
    enabledVk12Features.bufferDeviceAddress     = VK_TRUE;

    VkPhysicalDeviceVulkan13Features enabledVk13Features {};
    enabledVk13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    enabledVk13Features.pNext = &enabledVk12Features;
    enabledVk13Features.synchronization2 = VK_TRUE;
    enabledVk13Features.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceVulkan14Features enabledVk14Features {};
    enabledVk14Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
    enabledVk14Features.pNext = &enabledVk13Features;

    VkPhysicalDeviceFeatures enabledVk10Features {};
    enabledVk10Features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &enabledVk14Features;
    deviceCreateInfo.queueCreateInfoCount     = 1;
    deviceCreateInfo.pQueueCreateInfos        = &queueCreateInfo;;
    deviceCreateInfo.enabledExtensionCount    = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames  = deviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures         = &enabledVk10Features;

    if (vkCreateDevice(context.m_physicalDevice, &deviceCreateInfo, nullptr, &context.m_device) != VK_SUCCESS)
  throw std::runtime_error("failed to create logical device");

    vkGetDeviceQueue(context.m_device, queueFamily, 0, &context.m_queue);

    VmaVulkanFunctions vmaFunctions {};
    vmaFunctions.vkGetInstanceProcAddr  = vkGetInstanceProcAddr;
    vmaFunctions.vkGetDeviceProcAddr    = vkGetDeviceProcAddr;
    vmaFunctions.vkCreateImage          = vkCreateImage;

    VmaAllocatorCreateInfo vmaCreateInfo {};
    vmaCreateInfo.flags             = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateInfo.physicalDevice    = devices[deviceIndex];
    vmaCreateInfo.device            = context.m_device;
    vmaCreateInfo.pVulkanFunctions  = &vmaFunctions;
    vmaCreateInfo.instance          = context.m_instance;

    if (vmaCreateAllocator(&vmaCreateInfo, &context.m_allocator))
    {
      throw std::runtime_error("failed to load vma\n");
    }

    if (glfwCreateWindowSurface(context.m_instance, window.m_handle, NULL, &context.m_surface))
    {
      throw std::runtime_error("failed to create surface\n");
    }

    VkSurfaceCapabilitiesKHR surfaceCaps{};
    context.m_swapchain_extent = { window.m_width, window.m_height };

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.m_physicalDevice, context.m_surface, &surfaceCaps);

    uint32_t desiredImageCount = surfaceCaps.minImageCount + 1;
    if (surfaceCaps.maxImageCount > 0 && desiredImageCount > surfaceCaps.maxImageCount)
      desiredImageCount = surfaceCaps.maxImageCount;

    VkSwapchainCreateInfoKHR swapchainCreateInfo {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    context.m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;

    swapchainCreateInfo.surface         = context.m_surface;
    swapchainCreateInfo.minImageCount   = desiredImageCount;
    swapchainCreateInfo.imageFormat     = context.m_swapchainImageFormat;
    swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchainCreateInfo.imageExtent     = context.m_swapchain_extent;
    swapchainCreateInfo.imageArrayLayers  = 1;
    swapchainCreateInfo.imageUsage        = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.preTransform      = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha    = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode       = VK_PRESENT_MODE_FIFO_KHR;

    if (vkCreateSwapchainKHR(context.m_device, &swapchainCreateInfo, nullptr, &context.m_swapchain))
    {
      throw std::runtime_error("failed to create swapchain\n");
    }

    // retrieve swapchain images
    uint32_t swapchainImageCount = 0;
    vkGetSwapchainImagesKHR(context.m_device, context.m_swapchain, &swapchainImageCount, nullptr);
    context.m_swapchainImages.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(context.m_device, context.m_swapchain, &swapchainImageCount, context.m_swapchainImages.data());

    // create a view for each swapchain image
    context.m_swapchainImageViews.resize(swapchainImageCount);
    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
      VkImageViewCreateInfo viewCreateInfo{};
      viewCreateInfo.sType                            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      viewCreateInfo.image                             = context.m_swapchainImages[i];
      viewCreateInfo.viewType                          = VK_IMAGE_VIEW_TYPE_2D;
      viewCreateInfo.format                            = context.m_swapchainImageFormat;
      viewCreateInfo.subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
      viewCreateInfo.subresourceRange.baseMipLevel      = 0;
      viewCreateInfo.subresourceRange.levelCount        = 1;
      viewCreateInfo.subresourceRange.baseArrayLayer    = 0;
      viewCreateInfo.subresourceRange.layerCount        = 1;

      if (vkCreateImageView(context.m_device, &viewCreateInfo, nullptr, &context.m_swapchainImageViews[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create swapchain image view\n");
      }
    }


    // depth buffer 
    std::vector<VkFormat> depthFormatList {
      VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
    for (VkFormat& format : depthFormatList)
    {
    VkFormatProperties2 formatProperties { .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
    vkGetPhysicalDeviceFormatProperties2(devices[deviceIndex], format, &formatProperties);
    if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) 
    {
      depthFormat = format;
      break;
    }
    }

    context.m_depthFormat = depthFormat;

    VkImageCreateInfo depthImageCreateInfo {};
    depthImageCreateInfo.sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImageCreateInfo.imageType      = VK_IMAGE_TYPE_2D;
    depthImageCreateInfo.format         = depthFormat;
    depthImageCreateInfo.extent.width   = context.m_swapchain_extent.width; 
    depthImageCreateInfo.extent.height  = context.m_swapchain_extent.height; 
    depthImageCreateInfo.extent.depth   = 1; 
    depthImageCreateInfo.mipLevels      = 1;
    depthImageCreateInfo.arrayLayers    = 1;
    depthImageCreateInfo.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthImageCreateInfo.tiling         = VK_IMAGE_TILING_OPTIMAL;
    depthImageCreateInfo.usage          = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImageCreateInfo.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocCreateInfo{
      .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    vmaCreateImage(context.m_allocator, &depthImageCreateInfo, &allocCreateInfo,
               &context.m_depthImage, &context.m_depthAllocation, nullptr);

    VkImageViewCreateInfo depthViewCreateInfo{};
    depthViewCreateInfo.sType                         = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthViewCreateInfo.image                          = context.m_depthImage;
    depthViewCreateInfo.viewType                       = VK_IMAGE_VIEW_TYPE_2D;
    depthViewCreateInfo.format                         = context.m_depthFormat;
    depthViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    depthViewCreateInfo.subresourceRange.levelCount     = 1;
    depthViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    depthViewCreateInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(context.m_device, &depthViewCreateInfo, nullptr, &context.m_depthImageView) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create depth image view\n");
    }
  }

  void destoryContext(Context &context)
  {

    if (context.m_depthImageView != VK_NULL_HANDLE)
      vkDestroyImageView(context.m_device, context.m_depthImageView, nullptr);

    if (context.m_allocator != VK_NULL_HANDLE)
      vmaDestroyAllocator(context.m_allocator);

    for (VkImageView view : context.m_swapchainImageViews)
      if (view != VK_NULL_HANDLE)
        vkDestroyImageView(context.m_device, view, nullptr);

    if (context.m_swapchain != VK_NULL_HANDLE)
      vkDestroySwapchainKHR(context.m_device, context.m_swapchain, nullptr);

    if (context.m_surface != VK_NULL_HANDLE)
      vkDestroySurfaceKHR(context.m_instance, context.m_surface, nullptr);

    if (context.m_device != VK_NULL_HANDLE)
      vkDestroyDevice(context.m_device, nullptr);

    if (g_debug && context.m_debugMessenger != VK_NULL_HANDLE)
      pfnDestroyDebugUtilsMessengerEXT(context.m_instance, context.m_debugMessenger, nullptr);

    if (context.m_instance != VK_NULL_HANDLE)
      vkDestroyInstance(context.m_instance, nullptr);
  }
}
