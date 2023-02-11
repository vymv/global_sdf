#include "ez_vulkan.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <deque>

struct Context
{
    uint64_t frame_count = 0;
    VkDevice device = VK_NULL_HANDLE;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physical_device_properties{};
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties{};
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    uint32_t image_index = 0;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSemaphore acquire_semaphore = VK_NULL_HANDLE;
    VkSemaphore release_semaphore = VK_NULL_HANDLE;
} ctx;

struct ResourceManager
{
    uint64_t frame_count = 0;
    std::deque<std::pair<std::pair<VkImage, VkDeviceMemory>, uint64_t>> destroyer_images;
    std::deque<std::pair<VkImageView, uint64_t>> destroyer_imageviews;
    std::deque<std::pair<std::pair<VkBuffer, VkDeviceMemory>, uint64_t>> destroyer_buffers;
    std::deque<std::pair<VkSampler, uint64_t>> destroyer_samplers;
    std::deque<std::pair<VkDescriptorSetLayout, uint64_t>> destroyer_descriptor_set_layouts;
    std::deque<std::pair<VkDescriptorUpdateTemplate, uint64_t>> destroyer_descriptor_update_templates;
    std::deque<std::pair<VkShaderModule, uint64_t>> destroyer_shadermodules;
    std::deque<std::pair<VkPipelineLayout, uint64_t>> destroyer_pipeline_layouts;
    std::deque<std::pair<VkPipeline, uint64_t>> destroyer_pipelines;
} res_mgr;

void update_res_mgr(uint64_t current_frame_count)
{
    res_mgr.frame_count = current_frame_count;
    while (!res_mgr.destroyer_images.empty())
    {
        if (res_mgr.destroyer_images.front().second < res_mgr.frame_count)
        {
            auto item = res_mgr.destroyer_images.front();
            res_mgr.destroyer_images.pop_front();
            vkDestroyImage(ctx.device, item.first.first, nullptr);
            vkFreeMemory(ctx.device, item.first.second, nullptr);
        }
        else
        {
            break;
        }
    }
    while (!res_mgr.destroyer_imageviews.empty())
    {
        if (res_mgr.destroyer_imageviews.front().second < res_mgr.frame_count)
        {
            auto item = res_mgr.destroyer_imageviews.front();
            res_mgr.destroyer_imageviews.pop_front();
            vkDestroyImageView(ctx.device, item.first, nullptr);
        }
        else
        {
            break;
        }
    }
    while (!res_mgr.destroyer_buffers.empty())
    {
        if (res_mgr.destroyer_buffers.front().second < res_mgr.frame_count)
        {
            auto item = res_mgr.destroyer_buffers.front();
            res_mgr.destroyer_buffers.pop_front();
            vkDestroyBuffer(ctx.device, item.first.first, nullptr);
            vkFreeMemory(ctx.device, item.first.second, nullptr);
        }
        else
        {
            break;
        }
    }
    while (!res_mgr.destroyer_samplers.empty())
    {
        if (res_mgr.destroyer_samplers.front().second < res_mgr.frame_count)
        {
            auto item = res_mgr.destroyer_samplers.front();
            res_mgr.destroyer_samplers.pop_front();
            vkDestroySampler(ctx.device, item.first, nullptr);
        }
        else
        {
            break;
        }
    }
    while (!res_mgr.destroyer_descriptor_set_layouts.empty())
    {
        if (res_mgr.destroyer_descriptor_set_layouts.front().second < res_mgr.frame_count)
        {
            auto item = res_mgr.destroyer_descriptor_set_layouts.front();
            res_mgr.destroyer_descriptor_set_layouts.pop_front();
            vkDestroyDescriptorSetLayout(ctx.device, item.first, nullptr);
        }
        else
        {
            break;
        }
    }
    while (!res_mgr.destroyer_descriptor_update_templates.empty())
    {
        if (res_mgr.destroyer_descriptor_update_templates.front().second < res_mgr.frame_count)
        {
            auto item = res_mgr.destroyer_descriptor_update_templates.front();
            res_mgr.destroyer_descriptor_update_templates.pop_front();
            vkDestroyDescriptorUpdateTemplate(ctx.device, item.first, nullptr);
        }
        else
        {
            break;
        }
    }
    while (!res_mgr.destroyer_shadermodules.empty())
    {
        if (res_mgr.destroyer_shadermodules.front().second < res_mgr.frame_count)
        {
            auto item = res_mgr.destroyer_shadermodules.front();
            res_mgr.destroyer_shadermodules.pop_front();
            vkDestroyShaderModule(ctx.device, item.first, nullptr);
        }
        else
        {
            break;
        }
    }
    while (!res_mgr.destroyer_pipeline_layouts.empty())
    {
        if (res_mgr.destroyer_pipeline_layouts.front().second < res_mgr.frame_count)
        {
            auto item = res_mgr.destroyer_pipeline_layouts.front();
            res_mgr.destroyer_pipeline_layouts.pop_front();
            vkDestroyPipelineLayout(ctx.device, item.first, nullptr);
        }
        else
        {
            break;
        }
    }
    while (!res_mgr.destroyer_pipelines.empty())
    {
        if (res_mgr.destroyer_pipelines.front().second < res_mgr.frame_count)
        {
            auto item = res_mgr.destroyer_pipelines.front();
            res_mgr.destroyer_pipelines.pop_front();
            vkDestroyPipeline(ctx.device, item.first, nullptr);
        }
        else
        {
            break;
        }
    }
}

void clear_res_mgr()
{
    update_res_mgr(~0);
}

struct StageBufferPool
{
    uint64_t size = 0;
    uint64_t offset = 0;
    EzBuffer current_buffer = VK_NULL_HANDLE;
} stage_buffer_pool;

void clear_stage_buffer_pool()
{
    if (stage_buffer_pool.current_buffer != VK_NULL_HANDLE)
        ez_destroy_buffer(stage_buffer_pool.current_buffer);
}

#ifdef VK_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_cb(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                        VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                        void* user_data)
{
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        VK_LOGW("Validation Error %s: %s\n", callback_data->pMessageIdName, callback_data->pMessage);
    }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        VK_LOGE("Validation Warning %s: %s\n", callback_data->pMessageIdName, callback_data->pMessage);
    }
    return VK_FALSE;
}
#endif

static bool is_layer_supported(const char* required, const std::vector<VkLayerProperties>& available)
{
    for (const VkLayerProperties& availableLayer : available)
    {
        if (strcmp(availableLayer.layerName, required) == 0)
        {
            return true;
        }
    }
    return false;
}

static bool is_extension_supported(const char* required, const std::vector<VkExtensionProperties>& available)
{
    for (const VkExtensionProperties& available_extension : available)
    {
        if (strcmp(available_extension.extensionName, required) == 0)
        {
            return true;
        }
    }
    return false;
}

void ez_init()
{
    VK_ASSERT(volkInitialize());

    uint32_t num_instance_available_layers;
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&num_instance_available_layers, nullptr));
    std::vector<VkLayerProperties> instance_supported_layers(num_instance_available_layers);
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&num_instance_available_layers, instance_supported_layers.data()));

    uint32_t num_instance_available_extensions;
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &num_instance_available_extensions, nullptr));
    std::vector<VkExtensionProperties> instance_supported_extensions(num_instance_available_extensions);
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &num_instance_available_extensions, instance_supported_extensions.data()));

    std::vector<const char*> instance_required_layers;
    std::vector<const char*> instance_required_extensions;
    std::vector<const char*> instance_layers;
    std::vector<const char*> instance_extensions;

#ifdef VK_DEBUG
    instance_required_layers.push_back("VK_LAYER_KHRONOS_validation");
    instance_required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    instance_required_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef VK_USE_PLATFORM_WIN32_KHR
    instance_required_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

    for (auto it = instance_required_layers.begin(); it != instance_required_layers.end(); ++it)
    {
        if (is_layer_supported(*it, instance_supported_layers))
        {
            instance_layers.push_back(*it);
        }
    }

    for (auto it = instance_required_extensions.begin(); it != instance_required_extensions.end(); ++it)
    {
        if (is_extension_supported(*it, instance_supported_extensions))
        {
            instance_extensions.push_back(*it);
        }
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pEngineName = "vulkan";
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = nullptr;
    instance_create_info.flags = 0;
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.enabledLayerCount = instance_layers.size();
    instance_create_info.ppEnabledLayerNames = instance_layers.data();
    instance_create_info.enabledExtensionCount = instance_extensions.size();
    instance_create_info.ppEnabledExtensionNames = instance_extensions.data();

    VK_ASSERT(vkCreateInstance(&instance_create_info, nullptr, &ctx.instance));

    volkLoadInstance(ctx.instance);

#ifdef VK_DEBUG
    VkDebugUtilsMessengerCreateInfoEXT messenger_create_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    messenger_create_info.pfnUserCallback = debug_utils_messenger_cb;
    VK_ASSERT(vkCreateDebugUtilsMessengerEXT(ctx.instance, &messenger_create_info, nullptr, &ctx.debug_messenger));
#endif

    // Selected physical device
    uint32_t num_gpus = 0;
    VK_ASSERT(vkEnumeratePhysicalDevices(ctx.instance, &num_gpus, nullptr));
    std::vector<VkPhysicalDevice> gpus(num_gpus);
    VK_ASSERT(vkEnumeratePhysicalDevices(ctx.instance, &num_gpus, gpus.data()));
    for (auto& g : gpus)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(g, &props);
        VK_LOGI("Found Vulkan GPU: %s\n", props.deviceName);
        VK_LOGI("API: %u.%u.%u\n",
                VK_VERSION_MAJOR(props.apiVersion),
                VK_VERSION_MINOR(props.apiVersion),
                VK_VERSION_PATCH(props.apiVersion));
        VK_LOGI("Driver: %u.%u.%u\n",
                VK_VERSION_MAJOR(props.driverVersion),
                VK_VERSION_MINOR(props.driverVersion),
                VK_VERSION_PATCH(props.driverVersion));
    }
    ctx.physical_device = gpus.front();
    vkGetPhysicalDeviceProperties(ctx.physical_device, &ctx.physical_device_properties);
    vkGetPhysicalDeviceMemoryProperties(ctx.physical_device, &ctx.physical_device_memory_properties);

    // Features
    VkPhysicalDeviceFeatures2KHR physical_device_features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR};
    VkPhysicalDeviceVulkan11Features features_1_1 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features features_1_2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};

    VkPhysicalDeviceVulkan13Features features_1_3 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features_1_3.dynamicRendering = true;
    features_1_3.synchronization2 = true;
    features_1_3.maintenance4 = true;

    features_1_1.pNext = &features_1_2;
    features_1_2.pNext = &features_1_3;
    physical_device_features2.pNext = &features_1_1;
    vkGetPhysicalDeviceFeatures2(ctx.physical_device, &physical_device_features2);

    uint32_t num_queue_families;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physical_device, &num_queue_families, nullptr);
    std::vector<VkQueueFamilyProperties> queue_family_properties(num_queue_families);
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physical_device, &num_queue_families, queue_family_properties.data());
    uint32_t graphics_family = -1;
    for (uint32_t i = 0; i < (uint32_t)queue_family_properties.size(); i++)
    {
        if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphics_family = i;
            break;
        }
    }

    const float graphics_queue_prio = 0.0f;
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = graphics_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &graphics_queue_prio;

    uint32_t num_device_available_extensions = 0;
    VK_ASSERT(vkEnumerateDeviceExtensionProperties(ctx.physical_device, nullptr, &num_device_available_extensions, nullptr));
    std::vector<VkExtensionProperties> device_available_extensions(num_device_available_extensions);
    VK_ASSERT(vkEnumerateDeviceExtensionProperties(ctx.physical_device, nullptr, &num_device_available_extensions, device_available_extensions.data()));

    std::vector<const char*> device_required_extensions;
    std::vector<const char*> device_extensions;
    device_required_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    device_required_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    device_required_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    device_required_extensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
    device_required_extensions.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
    device_required_extensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
    device_required_extensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    for (auto it = device_required_extensions.begin(); it != device_required_extensions.end(); ++it)
    {
        if (is_extension_supported(*it, device_available_extensions))
        {
            device_extensions.push_back(*it);
        }
    }

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = &physical_device_features2;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.pEnabledFeatures = nullptr;
    device_create_info.enabledExtensionCount = device_extensions.size();
    device_create_info.ppEnabledExtensionNames = device_extensions.data();
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = nullptr;

    VK_ASSERT(vkCreateDevice(ctx.physical_device, &device_create_info, nullptr, &ctx.device));

    vkGetDeviceQueue(ctx.device, graphics_family, 0, &ctx.queue);

    VkCommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.queueFamilyIndex = graphics_family;
    cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    VK_ASSERT(vkCreateCommandPool(ctx.device, &cmd_pool_create_info, nullptr, &ctx.cmd_pool));

    VkCommandBufferAllocateInfo cmd_alloc_info = {};
    cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.commandBufferCount = 1;
    cmd_alloc_info.commandPool = ctx.cmd_pool;
    cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK_ASSERT(vkAllocateCommandBuffers(ctx.device, &cmd_alloc_info, &ctx.cmd));

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = nullptr;
    VK_ASSERT(vkBeginCommandBuffer(ctx.cmd, &begin_info));
}

void ez_terminate()
{
    clear_stage_buffer_pool();
    clear_res_mgr();
#ifdef VK_DEBUG
    if (ctx.debug_messenger != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(ctx.instance, ctx.debug_messenger, nullptr);
        ctx.debug_messenger = VK_NULL_HANDLE;
    }
#endif
    vkDestroyCommandPool(ctx.device, ctx.cmd_pool, nullptr);
    vkDestroyDevice(ctx.device, nullptr);
    vkDestroyInstance(ctx.instance, nullptr);
}

void ez_submit()
{
    vkEndCommandBuffer(ctx.cmd);

    VkPipelineStageFlags submit_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pWaitDstStageMask = &submit_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &ctx.cmd;
    if (ctx.acquire_semaphore != VK_NULL_HANDLE)
    {
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &ctx.acquire_semaphore;
    }
    if (ctx.release_semaphore != VK_NULL_HANDLE)
    {
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &ctx.release_semaphore;
    }

    VK_ASSERT(vkQueueSubmit(ctx.queue, 1, &submit_info, VK_NULL_HANDLE));

    if (ctx.swapchain != VK_NULL_HANDLE)
    {
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &ctx.release_semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &ctx.swapchain;
        present_info.pImageIndices = &ctx.image_index;

        VK_ASSERT(vkQueuePresentKHR(ctx.queue, &present_info));
    }

    VK_ASSERT(vkDeviceWaitIdle(ctx.device));

    VK_ASSERT(vkResetCommandPool(ctx.device, ctx.cmd_pool, 0));

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = nullptr;
    vkBeginCommandBuffer(ctx.cmd, &begin_info);

    ctx.frame_count++;
    update_res_mgr(ctx.frame_count);

    ctx.swapchain = VK_NULL_HANDLE;
    ctx.acquire_semaphore = VK_NULL_HANDLE;
    ctx.release_semaphore = VK_NULL_HANDLE;
}

VkCommandBuffer ez_cmd()
{
    return ctx.cmd;
}

VkDevice ez_device()
{
    return ctx.device;
}

void ez_wait_idle()
{
    vkDeviceWaitIdle(ctx.device);
}

// Swapchain
void ez_create_swapchain(void* window, EzSwapchain& swapchain)
{
    swapchain = new EzSwapchain_T();
#ifdef WIN32
    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.pNext = nullptr;
    surface_create_info.flags = 0;
    surface_create_info.hinstance = ::GetModuleHandle(nullptr);
    surface_create_info.hwnd = (HWND)window;
    VK_ASSERT(vkCreateWin32SurfaceKHR(ctx.instance, &surface_create_info, nullptr, &swapchain->surface));
#endif

    VkSurfaceCapabilitiesKHR surface_caps;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physical_device, swapchain->surface, &surface_caps));
    swapchain->width = surface_caps.currentExtent.width;
    swapchain->height = surface_caps.currentExtent.height;

    VkSwapchainCreateInfoKHR sc_create_info = {};
    sc_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sc_create_info.surface = swapchain->surface;
    sc_create_info.minImageCount = 2;
    sc_create_info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    sc_create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sc_create_info.imageExtent.width = swapchain->width;
    sc_create_info.imageExtent.height = swapchain->height;
    sc_create_info.imageArrayLayers = 1;
    sc_create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    sc_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    sc_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sc_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sc_create_info.oldSwapchain = VK_NULL_HANDLE;

    VK_ASSERT(vkCreateSwapchainKHR(ctx.device, &sc_create_info, nullptr, &swapchain->handle));

    VK_ASSERT(vkGetSwapchainImagesKHR(ctx.device, swapchain->handle, &swapchain->image_count, nullptr));
    swapchain->images.resize(swapchain->image_count);
    VK_ASSERT(vkGetSwapchainImagesKHR(ctx.device, swapchain->handle, &swapchain->image_count, swapchain->images.data()));

    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_ASSERT(vkCreateSemaphore(ctx.device, &semaphore_create_info, nullptr, &swapchain->acquire_semaphore));
    VK_ASSERT(vkCreateSemaphore(ctx.device, &semaphore_create_info, nullptr, &swapchain->release_semaphore));
}

void ez_destroy_swapchain(EzSwapchain swapchain)
{
    vkDestroySwapchainKHR(ctx.device, swapchain->handle, nullptr);
    vkDestroySurfaceKHR(ctx.instance, swapchain->surface, nullptr);
    vkDestroySemaphore(ctx.device, swapchain->acquire_semaphore, nullptr);
    vkDestroySemaphore(ctx.device, swapchain->release_semaphore, nullptr);
    delete swapchain;
}

EzSwapchainStatus ez_update_swapchain(EzSwapchain swapchain)
{
    VkSurfaceCapabilitiesKHR surface_caps;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physical_device, swapchain->surface, &surface_caps));
    uint32_t new_width = surface_caps.currentExtent.width;
    uint32_t new_height = surface_caps.currentExtent.height;

    if (new_width == 0 || new_height == 0)
        return EzSwapchainStatus::NotReady;

    if (swapchain->width == new_width && swapchain->height == new_height)
        return EzSwapchainStatus::Ready;

    swapchain->width = new_width;
    swapchain->height = new_height;
    VkSwapchainKHR old_handle = swapchain->handle;
    VkSwapchainCreateInfoKHR sc_create_info = {};
    sc_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sc_create_info.surface = swapchain->surface;
    sc_create_info.minImageCount = 2;
    sc_create_info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    sc_create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sc_create_info.imageExtent.width = swapchain->width;
    sc_create_info.imageExtent.height = swapchain->height;
    sc_create_info.imageArrayLayers = 1;
    sc_create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    sc_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    sc_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sc_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sc_create_info.oldSwapchain = old_handle;

    VK_ASSERT(vkCreateSwapchainKHR(ctx.device, &sc_create_info, nullptr, &swapchain->handle));

    VK_ASSERT(vkGetSwapchainImagesKHR(ctx.device, swapchain->handle, &swapchain->image_count, nullptr));
    swapchain->images.resize(swapchain->image_count);
    VK_ASSERT(vkGetSwapchainImagesKHR(ctx.device, swapchain->handle, &swapchain->image_count, swapchain->images.data()));

    vkDestroySwapchainKHR(ctx.device, old_handle, nullptr);

    return EzSwapchainStatus::Resized;
}

void ez_acquire_next_image(EzSwapchain swapchain)
{
    vkAcquireNextImageKHR(ctx.device, swapchain->handle, ~0ull, swapchain->acquire_semaphore, VK_NULL_HANDLE, &swapchain->image_index);
    ctx.image_index = swapchain->image_index;
    ctx.acquire_semaphore = swapchain->acquire_semaphore;
}

void ez_present(EzSwapchain swapchain)
{
    ctx.swapchain = swapchain->handle;
    ctx.release_semaphore = swapchain->release_semaphore;
}

// Buffer
static uint32_t select_memory_type(const VkPhysicalDeviceMemoryProperties& memory_properties, uint32_t memory_type_bits, VkMemoryPropertyFlags flags)
{
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
        if ((memory_type_bits & (1 << i)) != 0 && (memory_properties.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    return ~0u;
}

void ez_create_buffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_flags, EzBuffer& buffer)
{
    buffer = new EzBuffer_T();
    buffer->size = size;

    VkBufferCreateInfo buffer_create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    VK_ASSERT(vkCreateBuffer(ctx.device, &buffer_create_info, nullptr, &buffer->handle));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(ctx.device, buffer->handle, &memory_requirements);
    uint32_t memory_type_tndex = select_memory_type(ctx.physical_device_memory_properties, memory_requirements.memoryTypeBits, memory_flags);

    VkMemoryAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = memory_type_tndex;
    VK_ASSERT(vkAllocateMemory(ctx.device, &allocate_info, nullptr, &buffer->memory));
    VK_ASSERT(vkBindBufferMemory(ctx.device, buffer->handle, buffer->memory, 0));
}

void ez_destroy_buffer(EzBuffer buffer)
{
    res_mgr.destroyer_buffers.emplace_back(std::make_pair(buffer->handle, buffer->memory), ctx.frame_count);
    delete buffer;
}

EzAllocation ez_alloc_stage_buffer(size_t size)
{
    const uint64_t free_space = stage_buffer_pool.size - stage_buffer_pool.offset;
    if (size > free_space || stage_buffer_pool.current_buffer == VK_NULL_HANDLE)
    {
        if (stage_buffer_pool.current_buffer != VK_NULL_HANDLE)
        {
            ez_destroy_buffer(stage_buffer_pool.current_buffer);
        }

        stage_buffer_pool.size = ez_align_to((stage_buffer_pool.size + size) * 2, 8);
        stage_buffer_pool.offset = 0;

        ez_create_buffer(stage_buffer_pool.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         stage_buffer_pool.current_buffer);
    }

    EzAllocation allocation;
    allocation.buffer = stage_buffer_pool.current_buffer;
    allocation.offset = stage_buffer_pool.offset;
    stage_buffer_pool.offset += ez_align_to(size, 8);
    return allocation;
}

// Barrier
VkImageMemoryBarrier2 ez_image_barrier(VkImage image,
                                    VkPipelineStageFlags2 src_stage_mask, VkAccessFlags2 src_access_mask, VkImageLayout old_layout,
                                    VkPipelineStageFlags2 dst_stage_mask, VkAccessFlags2 dst_access_mask, VkImageLayout new_layout,
                                    VkImageAspectFlags aspect_mask)
{
    VkImageMemoryBarrier2 barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = src_stage_mask;
    barrier.srcAccessMask = src_access_mask;
    barrier.dstStageMask = dst_stage_mask;
    barrier.dstAccessMask = dst_access_mask;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect_mask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return barrier;
}

VkBufferMemoryBarrier2 ez_buffer_barrier(VkBuffer buffer,
                                      VkPipelineStageFlags2 src_stage_mask, VkAccessFlags2 src_access_mask,
                                      VkPipelineStageFlags2 dst_stage_mask, VkAccessFlags2 dst_access_mask)
{
    VkBufferMemoryBarrier2 barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barrier.srcStageMask = src_stage_mask;
    barrier.srcAccessMask = src_access_mask;
    barrier.dstStageMask = dst_stage_mask;
    barrier.dstAccessMask = dst_access_mask;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = buffer;
    barrier.offset = 0;
    barrier.size = VK_WHOLE_SIZE;

    return barrier;
}

void ez_pipeline_barrier(VkCommandBuffer cmd, VkDependencyFlags dependency_flags,
                      size_t buffer_barrier_count, const VkBufferMemoryBarrier2* buffer_barriers,
                      size_t image_barrier_count, const VkImageMemoryBarrier2* image_barriers)
{
    VkDependencyInfo dependency_info = {};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.dependencyFlags = dependency_flags;
    dependency_info.bufferMemoryBarrierCount = unsigned(buffer_barrier_count);
    dependency_info.pBufferMemoryBarriers = buffer_barriers;
    dependency_info.imageMemoryBarrierCount = unsigned(image_barrier_count);
    dependency_info.pImageMemoryBarriers = image_barriers;

    vkCmdPipelineBarrier2(cmd, &dependency_info);
}