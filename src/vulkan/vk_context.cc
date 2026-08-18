#include "vk_context.h"
#include "src/vulkan/window.h"
#include <set>
#include <vulkan/vulkan_core.h>

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;

  bool is_complete() {
    return graphics_family.has_value();
  }
};

struct SwapchainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

const std::vector<const char *> validation_layers = {
  "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_extensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
  std::print("\e[0;33m" "validation layer: " "\e[0m" "{}\n", pCallbackData->pMessage);

  return VK_FALSE;
}

VkResult create_debug_utils_messenger_ext(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* messengerCI,
    const VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* debug_messenger)
{
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
      "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, messengerCI, allocator, debug_messenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void destroy_debug_utils_messenger_ext(VkInstance instance,
    VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks* allocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debug_messenger, allocator);
  }
}

void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& messengerCI) {
  messengerCI = {};
  messengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  messengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  messengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  messengerCI.pfnUserCallback = debugCallback;
}

std::vector<const char*> get_required_extensions() {
  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

  if (enable_validation_layers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

bool check_validation_layer_support() {
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  for (const char* layer_name : validation_layers) {
    bool layer_found = false;

    for (const auto& layer_properties : available_layers) {
      if (strcmp(layer_name, layer_properties.layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      return false;
    }
  }

  return true;
}

SwapchainSupportDetails query_swapchain_support(Vk_Context &context, VkPhysicalDevice device) {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context.m_surface, &details.capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.m_surface, &format_count, nullptr);

  if (format_count != 0) {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.m_surface, &format_count, details.formats.data());
  }

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.m_surface, &present_mode_count, nullptr);

  if (present_mode_count != 0) {
    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.m_surface, &present_mode_count, details.present_modes.data());
  }

  return details;
}

QueueFamilyIndices find_queue_families(VkPhysicalDevice device, Vk_Context &context) {
  QueueFamilyIndices indices;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

  int i = 0;
  for (const auto& queue_family : queue_families) {
    if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphics_family = i;
    }

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context.m_surface, &present_support);

    if (present_support) {
      indices.present_family = i;
    }

    if (indices.is_complete()) {
      break;
    }

    i++;
  }

  return indices;
}

bool is_device_suitable(VkPhysicalDevice device, Vk_Context &context) {
  QueueFamilyIndices indices = find_queue_families(device, context);

  return indices.is_complete();
}

VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
  for (const auto& available_format : available_formats) {
    if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return available_format;
    }
  }

  return available_formats[0];
}

VkPresentModeKHR choose_swap_present_mode(Vk_Context &context, const std::vector<VkPresentModeKHR>& available_present_modes)
{
  if (context.m_vsync)
  {
    return VK_PRESENT_MODE_FIFO_KHR;
  }
  for (const auto& available_present_mode : available_present_modes) {
    if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return available_present_mode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D choose_swap_extent(Vk_Window &window, const VkSurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window.m_handle, &width, &height);

    VkExtent2D actual_extent = {
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
    };

    actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actual_extent;
  }
}


namespace n_context {
  void create_instance(Vk_Context &context) {
    VkApplicationInfo app_info {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_3
    };

    // create info
    VkInstanceCreateInfo instanceCI{};
    instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCI.pApplicationInfo = &app_info;

    auto extensions = get_required_extensions();
    instanceCI.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceCI.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT messengerCI{};
    if (enable_validation_layers && check_validation_layer_support()) {
      instanceCI.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
      instanceCI.ppEnabledLayerNames = validation_layers.data();

      populate_debug_messenger_create_info(messengerCI);
      instanceCI.pNext = &messengerCI;
    } else {
      instanceCI.enabledLayerCount = 0;
      instanceCI.pNext = nullptr;
    }

    // create it in memory
    if (vkCreateInstance(&instanceCI, nullptr, &context.m_instance) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create instance");
    }
  }

  void create_message(Vk_Context &context) {
    if (!enable_validation_layers) return;

    VkDebugUtilsMessengerCreateInfoEXT messengerCI;
    populate_debug_messenger_create_info(messengerCI);

    if (create_debug_utils_messenger_ext(context.m_instance, &messengerCI, nullptr, &context.m_debug_messenger) != VK_SUCCESS) {
      throw std::runtime_error("failed to set up debug messenger");
    }
  }

  void create_device(Vk_Context &context) {
    QueueFamilyIndices indices = find_queue_families(context.m_physical_device, context);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(),
      indices.present_family.value()};

    float queue_priority = 1.0f;
    for (uint32_t queue_family : unique_queue_families) {
      VkDeviceQueueCreateInfo queueCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .queueFamilyIndex = queue_family,
          .queueCount = 1,
          .pQueuePriorities = &queue_priority,
      };
      queue_create_infos.push_back(queueCI);
    }

    VkPhysicalDeviceVulkan12Features features12 {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
        .bufferDeviceAddress = true,
    };

    VkPhysicalDeviceVulkan13Features features13 {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &features12,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE
    };

    VkPhysicalDeviceFeatures features10{
      .samplerAnisotropy = VK_TRUE,
    };

    VkDeviceCreateInfo deviceCI {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features13,
        .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = &features10
    };

    if (vkCreateDevice(context.m_physical_device, &deviceCI, nullptr, &context.m_device) != VK_SUCCESS) {
      throw std::runtime_error("failed to create logical device");
    }

    vkGetDeviceQueue(context.m_device, indices.graphics_family.value(), 0, &context.m_graphics_queue);
    vkGetDeviceQueue(context.m_device, indices.present_family.value(), 0, &context.m_present_queue);
  }

  void create_physical_device(Vk_Context &context) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(context.m_instance, &device_count, nullptr);

    if (device_count == 0) {
      throw std::runtime_error("failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(context.m_instance, &device_count, devices.data());

    for (const auto& device : devices) {
      if (is_device_suitable(device, context)) {
        context.m_physical_device = device;
        break;
      }
    }

    if (context.m_physical_device == VK_NULL_HANDLE) {
      throw std::runtime_error("failed to find a suitable GPU");
    }

    VkPhysicalDeviceProperties2 device_properties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    vkGetPhysicalDeviceProperties2(context.m_physical_device, &device_properties);
    std::print("\e[0;32m" "selected device: " "\e[0m" "{}\n", device_properties.properties.deviceName);
  }

  void create_surface(Vk_Context &context, Vk_Window &window)
  {
    if (glfwCreateWindowSurface(context.m_instance, window.m_handle, nullptr, &context.m_surface) != VK_SUCCESS) {
      throw std::runtime_error("failed to create window surface");
    }
  }

  void create_allocator(Vk_Context &context)
  {
    VmaAllocatorCreateInfo allocatorCI{
      .flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = context.m_physical_device,
        .device         = context.m_device,
        .instance       = context.m_instance,
    };

    if (vmaCreateAllocator(&allocatorCI, &context.m_allocator) != VK_SUCCESS) {
      throw std::runtime_error("failed to create vma allocator");
    }
  }


  void create_swapchain(Vk_Context &context, Vk_Window &window)
  {
    SwapchainSupportDetails swapchain_support = query_swapchain_support(context ,context.m_physical_device);

    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
    VkPresentModeKHR present_mode     = choose_swap_present_mode(context, swapchain_support.present_modes);
    VkExtent2D extent                 = choose_swap_extent(window, swapchain_support.capabilities);

    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount) {
      image_count = swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCI{};
    swapchainCI.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.surface          = context.m_surface;
    swapchainCI.minImageCount    = image_count;
    swapchainCI.imageFormat      = surface_format.format;
    swapchainCI.imageColorSpace  = surface_format.colorSpace;
    swapchainCI.imageExtent      = extent;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices    = find_queue_families(context.m_physical_device, context);
    uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};

    if (indices.graphics_family        != indices.present_family) {
      swapchainCI.imageSharingMode       = VK_SHARING_MODE_CONCURRENT;
      swapchainCI.queueFamilyIndexCount  = 2;
      swapchainCI.pQueueFamilyIndices    = queue_family_indices;
    } else {
      swapchainCI.imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapchainCI.preTransform   = swapchain_support.capabilities.currentTransform;
    swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCI.presentMode    = present_mode;
    swapchainCI.clipped        = VK_TRUE;
    swapchainCI.oldSwapchain   = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(context.m_device, &swapchainCI, nullptr, &context.m_swapchain) != VK_SUCCESS) {
      throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(context.m_device, context.m_swapchain, &image_count, nullptr);
    context.m_swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(context.m_device, context.m_swapchain, &image_count, context.m_swapchain_images.data());

    context.m_swapchain_image_format = surface_format.format;
    context.m_swapchain_extent       = extent;
  }

  void create_image_views(Vk_Context &context)
  {
    context.m_swapchain_image_views.resize(context.m_swapchain_images.size());

    for (size_t i = 0; i < context.m_swapchain_images.size(); i++) {
      VkImageViewCreateInfo viewCI{};
      viewCI.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      viewCI.image    = context.m_swapchain_images[i];
      viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
      viewCI.format   = context.m_swapchain_image_format;

      viewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      viewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      viewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      viewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

      viewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      viewCI.subresourceRange.baseMipLevel   = 0;
      viewCI.subresourceRange.levelCount     = 1;
      viewCI.subresourceRange.baseArrayLayer = 0;
      viewCI.subresourceRange.layerCount     = 1;

      if (vkCreateImageView(context.m_device, &viewCI, nullptr,
            &context.m_swapchain_image_views[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image views!");
      }
    }

  }

  void create(Vk_Context &vk_context, Vk_Window &vk_window)
  {
    create_instance(vk_context);
    create_message(vk_context);
    create_surface(vk_context, vk_window);
    create_physical_device(vk_context);
    create_device(vk_context);
    create_allocator(vk_context);
    create_swapchain(vk_context, vk_window);
    create_image_views(vk_context);

    std::print("\e[0;32m" "pass: " "\e[0m" "vk context\n");
  }

  void destroy_image_views(Vk_Context &context)
  {
    for (auto image_view : context.m_swapchain_image_views)
      if (image_view != VK_NULL_HANDLE)
        vkDestroyImageView(context.m_device, image_view, nullptr);
    context.m_swapchain_image_views.clear();
  }

  void destroy_swapchain(Vk_Context &vk_context)
  {
    destroy_image_views(vk_context);
    if (vk_context.m_swapchain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR(vk_context.m_device, vk_context.m_swapchain, nullptr);
      vk_context.m_swapchain = VK_NULL_HANDLE;
    }
  }

  void destroy(Vk_Context &context)
  {
    if (context.m_allocator != VK_NULL_HANDLE)
      vmaDestroyAllocator(context.m_allocator);

    destroy_swapchain(context);

    if (context.m_surface != VK_NULL_HANDLE)
      vkDestroySurfaceKHR(context.m_instance, context.m_surface, nullptr);

    if (context.m_device != VK_NULL_HANDLE)
      vkDestroyDevice(context.m_device, nullptr);

    if (enable_validation_layers && context.m_debug_messenger != VK_NULL_HANDLE)
      destroy_debug_utils_messenger_ext(context.m_instance, context.m_debug_messenger, nullptr);

    if (context.m_instance != VK_NULL_HANDLE)
      vkDestroyInstance(context.m_instance, nullptr);
  }

} // namespace n_context
