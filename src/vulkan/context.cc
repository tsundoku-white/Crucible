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
      std::runtime_error("failed to load vk instance\n");
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

    vkGetDeviceQueue(context.m_device, queueFamily, queueFamilyCount, &context.m_queue);

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
      std::runtime_error("failed to load vma\n");
    }

    if (glfwCreateWindowSurface(context.m_instance, window.m_handle, NULL, &context.m_surface))
    {
      std::runtime_error("failed to create surface\n");
    }

    VkSurfaceCapabilitiesKHR surfaceCaps{};
    VkExtent2D swapchainExtent{ window.m_width, window.m_height };

    VkSwapchainCreateInfoKHR swapchainCreateInfo {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface         = context.m_surface;
    swapchainCreateInfo.minImageCount   = surfaceCaps.minImageCount;
    swapchainCreateInfo.imageFormat     = VK_FORMAT_B8G8R8A8_SRGB;
    swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchainCreateInfo.imageExtent     = {
      .width = swapchainExtent.width,
      .height = swapchainExtent.height
    };
    swapchainCreateInfo.imageArrayLayers  = 1;
    swapchainCreateInfo.imageUsage        = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.preTransform      = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha    = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode       = VK_PRESENT_MODE_FIFO_KHR;

    if (vkCreateSwapchainKHR(context.m_device, &swapchainCreateInfo, nullptr, &context.m_swapchain))
    {
      std::runtime_error("failed to create swapchain\n");
    }
  }

  void destoryContext(Context &context)
  {

    if (context.m_allocator != VK_NULL_HANDLE)
      vmaDestroyAllocator(context.m_allocator);

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
