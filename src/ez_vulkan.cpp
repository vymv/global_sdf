#include "ez_vulkan.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <deque>
#include <spirv_reflect.h>

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
    EzGraphicsPipeline graphics_pipeline = VK_NULL_HANDLE;
    EzComputePipeline compute_pipeline = VK_NULL_HANDLE;
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
    instance_create_info.enabledLayerCount = (uint32_t)instance_layers.size();
    instance_create_info.ppEnabledLayerNames = instance_layers.data();
    instance_create_info.enabledExtensionCount = (uint32_t)instance_extensions.size();
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
    device_create_info.enabledExtensionCount = (uint32_t)device_extensions.size();
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

void ez_create_buffer(const EzBufferDesc& desc, EzBuffer& buffer)
{
    buffer = new EzBuffer_T();
    buffer->size = desc.size;

    VkBufferCreateInfo buffer_create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = desc.size;
    buffer_create_info.usage = desc.usage;
    VK_ASSERT(vkCreateBuffer(ctx.device, &buffer_create_info, nullptr, &buffer->handle));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(ctx.device, buffer->handle, &memory_requirements);
    uint32_t memory_type_tndex = select_memory_type(ctx.physical_device_memory_properties, memory_requirements.memoryTypeBits, desc.memory_flags);

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

        EzBufferDesc buffer_desc = {};
        buffer_desc.size = stage_buffer_pool.size;
        buffer_desc.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_desc.memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        ez_create_buffer(buffer_desc, stage_buffer_pool.current_buffer);
    }

    EzAllocation allocation;
    allocation.buffer = stage_buffer_pool.current_buffer;
    allocation.offset = stage_buffer_pool.offset;
    stage_buffer_pool.offset += ez_align_to(size, 8);
    return allocation;
}

// Texture
void ez_create_texture(const EzTextureDesc& desc, EzTexture& texture)
{
    texture = new EzTexture_T();
    texture->width = desc.width;
    texture->height = desc.height;
    texture->depth = desc.depth;
    texture->levels = desc.levels;
    texture->layers = desc.layers;
    texture->format = desc.format;
    texture->layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = desc.image_type;
    image_create_info.format = desc.format;
    image_create_info.extent.width = desc.width;
    image_create_info.extent.height = desc.height;
    image_create_info.extent.depth = desc.depth;
    image_create_info.mipLevels = desc.levels;
    image_create_info.arrayLayers = desc.layers;
    image_create_info.samples = desc.samples;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = nullptr;
    image_create_info.flags = 0;
    image_create_info.usage = desc.usage;
    VK_ASSERT(vkCreateImage(ctx.device, &image_create_info, nullptr, &texture->handle));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(ctx.device, texture->handle, &memory_requirements);
    uint32_t memoryTypeIndex = select_memory_type(ctx.physical_device_memory_properties, memory_requirements.memoryTypeBits, desc.memory_flags);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = memoryTypeIndex;

    VK_ASSERT(vkAllocateMemory(ctx.device, &allocate_info, nullptr, &texture->memory));
    VK_ASSERT(vkBindImageMemory(ctx.device, texture->handle, texture->memory, 0));
}

void ez_destroy_texture(EzTexture texture)
{
    res_mgr.destroyer_images.emplace_back(std::make_pair(texture->handle, texture->memory), ctx.frame_count);
    for (auto view : texture->views)
    {
        res_mgr.destroyer_imageviews.emplace_back(view, ctx.frame_count);
    }
    delete texture;
}

int ez_create_texture_view(EzTexture texture, VkImageViewType view_type,
                           uint32_t base_level, uint32_t level_count,
                           uint32_t base_layer, uint32_t layer_count)
{
    VkImageViewCreateInfo view_create_info = {};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.flags = 0;
    view_create_info.image = texture->handle;
    view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_create_info.subresourceRange.baseArrayLayer = base_layer;
    view_create_info.subresourceRange.layerCount = layer_count;
    view_create_info.subresourceRange.baseMipLevel = base_level;
    view_create_info.subresourceRange.levelCount = level_count;
    view_create_info.format = texture->format;
    view_create_info.viewType = view_type;

    switch (view_create_info.format)
    {
        case VK_FORMAT_D16_UNORM:
            view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        case VK_FORMAT_D32_SFLOAT:
            view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        case VK_FORMAT_D24_UNORM_S8_UINT:
            view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
    }
    VkImageView image_view;
    VK_ASSERT(vkCreateImageView(ctx.device, &view_create_info, nullptr, &image_view));

    texture->views.push_back(image_view);
    return int(texture->views.size()) - 1;
}

void ez_create_sampler(const EzSamplerDesc& desc, EzSampler& sampler)
{
    sampler = new EzSampler_T();

    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = desc.mag_filter;
    sampler_create_info.minFilter = desc.min_filter;
    sampler_create_info.mipmapMode = desc.mipmap_mode;
    sampler_create_info.addressModeU = desc.address_u;
    sampler_create_info.addressModeV = desc.address_v;
    sampler_create_info.addressModeW = desc.address_w;
    sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_create_info.anisotropyEnable = desc.anisotropy_enable;
    sampler_create_info.maxAnisotropy = 0.0f;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    VK_ASSERT(vkCreateSampler(ctx.device, &sampler_create_info, nullptr, &sampler->handle));
}

void ez_destroy_sampler(EzSampler sampler)
{
    res_mgr.destroyer_samplers.emplace_back(std::make_pair(sampler->handle, ctx.frame_count));
    delete sampler;
}

// Pipeline
static VkShaderStageFlagBits parse_shader_stage(SpvReflectShaderStageFlagBits reflect_shader_stage)
{
    switch (reflect_shader_stage)
    {
        case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            return VkShaderStageFlagBits(0);
    }
}

void ez_create_shader(void* data, size_t size, EzShader& shader)
{
    shader = new EzShader_T();

    VkShaderModuleCreateInfo shader_create_info = {};
    shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.codeSize = size;
    shader_create_info.pCode = (const uint32_t*)data;
    VK_ASSERT(vkCreateShaderModule(ctx.device, &shader_create_info, nullptr, &shader->handle));

    // Parse shader
    SpvReflectShaderModule reflect_shader_module;
    SpvReflectResult reflect_result = spvReflectCreateShaderModule(shader_create_info.codeSize, shader_create_info.pCode, &reflect_shader_module);

    shader->stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader->stage_info.module = shader->handle;
    shader->stage_info.pName = "main";
    shader->stage_info.stage = parse_shader_stage(reflect_shader_module.shader_stage);

    uint32_t binding_count = 0;
    reflect_result = spvReflectEnumerateDescriptorBindings(&reflect_shader_module, &binding_count, nullptr);

    std::vector<SpvReflectDescriptorBinding*> bindings(binding_count);
    reflect_result = spvReflectEnumerateDescriptorBindings(&reflect_shader_module, &binding_count, bindings.data());

    uint32_t push_count = 0;
    reflect_result = spvReflectEnumeratePushConstantBlocks(&reflect_shader_module, &push_count, nullptr);

    std::vector<SpvReflectBlockVariable*> pushconstants(push_count);
    reflect_result = spvReflectEnumeratePushConstantBlocks(&reflect_shader_module, &push_count, pushconstants.data());

    for (auto& x : pushconstants)
    {
        VkPushConstantRange pushconstant = {};
        pushconstant.stageFlags = shader->stage_info.stage;
        pushconstant.offset = x->offset;
        pushconstant.size = x->size;
        shader->pushconstants.push_back(pushconstant);
    }

    for (auto& x : bindings)
    {
        VkDescriptorSetLayoutBinding descriptor = {};
        descriptor.stageFlags = shader->stage_info.stage;
        descriptor.binding = x->binding;
        descriptor.descriptorCount = x->count;
        descriptor.descriptorType = (VkDescriptorType)x->descriptor_type;
        shader->layout_bindings.push_back(descriptor);
    }

    spvReflectDestroyShaderModule(&reflect_shader_module);
}

void ez_destroy_shader(EzShader shader)
{
    res_mgr.destroyer_shadermodules.emplace_back(shader->handle, ctx.frame_count);
    delete shader;
}

uint32_t GetFormatStride(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
            return 16;

        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
            return 12;

        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
            return 8;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return 4;

        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R16_SFLOAT:
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_SINT:
            return 2;

        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_SINT:
            return 1;

        default:
            break;
    }

    return 0;
}

void ez_create_graphics_pipeline(const EzGraphicsPipelineDesc& desc, EzGraphicsPipeline& pipeline)
{
    pipeline = new EzGraphicsPipeline_T();

    // Pipeline layout
    auto insert_shader = [&](EzShader shader)
    {
        if (shader == VK_NULL_HANDLE)
            return;

        uint32_t i = 0;
        for (auto& x : shader->layout_bindings)
        {
            bool found = false;
            for (auto& y : pipeline->layout_bindings)
            {
                if (x.binding == y.binding)
                {
                    found = true;
                    y.stageFlags |= x.stageFlags;
                    break;
                }
            }

            if (!found)
            {
                pipeline->layout_bindings.push_back(x);
            }
            i++;
        }

        for (auto& x : shader->pushconstants)
        {
            pipeline->pushconstants.push_back(x);
        }
    };

    insert_shader(desc.vertex_shader);
    insert_shader(desc.fragment_shader);

    std::vector<VkDescriptorSetLayout> set_layouts;
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pBindings = pipeline->layout_bindings.data();
    descriptor_set_layout_create_info.bindingCount = (uint32_t)pipeline->layout_bindings.size();
    VK_ASSERT(vkCreateDescriptorSetLayout(ctx.device, &descriptor_set_layout_create_info, nullptr, &pipeline->descriptor_set_layout));
    set_layouts.push_back(pipeline->descriptor_set_layout);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pSetLayouts = set_layouts.data();
    pipeline_layout_create_info.setLayoutCount = (uint32_t)set_layouts.size();
    if (!pipeline->pushconstants.empty())
    {
        pipeline_layout_create_info.pushConstantRangeCount = (uint32_t)pipeline->pushconstants.size();
        pipeline_layout_create_info.pPushConstantRanges = pipeline->pushconstants.data();
    }
    else
    {
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_layout_create_info.pPushConstantRanges = nullptr;
    }
    VK_ASSERT(vkCreatePipelineLayout(ctx.device, &pipeline_layout_create_info, nullptr, &pipeline->pipeline_layout));

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.layout = pipeline->pipeline_layout;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    // Shader
    uint32_t shader_stage_count = 0;
    VkPipelineShaderStageCreateInfo shader_stages[2] = {};
    if (desc.vertex_shader != VK_NULL_HANDLE)
    {
        shader_stages[shader_stage_count++] = desc.vertex_shader->stage_info;
    }
    if (desc.fragment_shader != VK_NULL_HANDLE)
    {
        shader_stages[shader_stage_count++] = desc.fragment_shader->stage_info;
    }
    pipeline_info.stageCount = shader_stage_count;
    pipeline_info.pStages = shader_stages;

    // Input
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = desc.input_assembly.topology;
    input_assembly.primitiveRestartEnable = VK_FALSE;
    pipeline_info.pInputAssemblyState = &input_assembly;

    uint32_t num_input_bindings = 0;
    VkVertexInputBindingDescription input_bindings[15] = {{0}};
    uint32_t num_input_attributes = 0;
    VkVertexInputAttributeDescription input_attributes[15] = {{0}};
    uint32_t binding_value = UINT32_MAX;
    for (auto& element : desc.input_layout.elements)
    {
        if (binding_value != element.binding)
        {
            binding_value = element.binding;
            ++num_input_bindings;
        }
        input_bindings[num_input_bindings - 1].binding = binding_value;
        input_bindings[num_input_bindings - 1].inputRate = element.rate;
        input_bindings[num_input_bindings - 1].stride += GetFormatStride(element.format);
        input_attributes[num_input_attributes].location = element.location;
        input_attributes[num_input_attributes].binding = element.binding;
        input_attributes[num_input_attributes].format = element.format;
        input_attributes[num_input_attributes].offset = element.offset;
        ++num_input_attributes;
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = num_input_bindings;
    vertex_input_info.pVertexBindingDescriptions = input_bindings;
    vertex_input_info.vertexAttributeDescriptionCount = num_input_attributes;
    vertex_input_info.pVertexAttributeDescriptions = input_attributes;
    pipeline_info.pVertexInputState = &vertex_input_info;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_TRUE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = desc.rasterization_state.fill_mode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = desc.rasterization_state.cull_mode;
    rasterizer.frontFace = desc.rasterization_state.front_face;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;
    pipeline_info.pRasterizationState = &rasterizer;

    // MSAA
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = desc.multisample_state.samples;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
    pipeline_info.pMultisampleState = &multisampling;

    // Blend
    uint32_t num_blend_attachments = 0;
    VkPipelineColorBlendAttachmentState blend_attachments[4] = {};
    for (uint32_t i = 0; i < 4; ++i)
    {
        VkPipelineColorBlendAttachmentState& attachment = blend_attachments[num_blend_attachments];
        attachment.blendEnable = desc.blend_state.blend_enable ? VK_TRUE : VK_FALSE;
        attachment.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
        attachment.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
        attachment.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
        attachment.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
        attachment.srcColorBlendFactor = desc.blend_state.src_color;
        attachment.dstColorBlendFactor = desc.blend_state.dst_color;
        attachment.colorBlendOp = desc.blend_state.color_op;
        attachment.srcAlphaBlendFactor = desc.blend_state.src_alpha;
        attachment.dstAlphaBlendFactor = desc.blend_state.dst_alpha;
        attachment.alphaBlendOp = desc.blend_state.alpha_op;
        num_blend_attachments++;
    }

    VkPipelineColorBlendStateCreateInfo blending_info = {};
    blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blending_info.logicOpEnable = VK_FALSE;
    blending_info.logicOp = VK_LOGIC_OP_COPY;
    blending_info.attachmentCount = num_blend_attachments;
    blending_info.pAttachments = blend_attachments;
    blending_info.blendConstants[0] = 1.0f;
    blending_info.blendConstants[1] = 1.0f;
    blending_info.blendConstants[2] = 1.0f;
    blending_info.blendConstants[3] = 1.0f;
    pipeline_info.pColorBlendState = &blending_info;

    // Tessellation
    VkPipelineTessellationStateCreateInfo tessellation_info = {};
    tessellation_info.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellation_info.patchControlPoints = 3;
    pipeline_info.pTessellationState = &tessellation_info;

    // Viewport state
    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = nullptr;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = nullptr;
    pipeline_info.pViewportState = &viewport_state;

    // Dynamic states
    VkDynamicState dynamic_states[5];
    dynamic_states[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamic_states[1] = VK_DYNAMIC_STATE_SCISSOR;
    dynamic_states[2] = VK_DYNAMIC_STATE_DEPTH_BIAS;
    dynamic_states[3] = VK_DYNAMIC_STATE_BLEND_CONSTANTS;
    dynamic_states[4] = VK_DYNAMIC_STATE_DEPTH_BOUNDS;

    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.flags = 0;
    dynamic_state.dynamicStateCount = 5;
    dynamic_state.pDynamicStates = dynamic_states;
    pipeline_info.pDynamicState = &dynamic_state;

    // Renderpass layout
    VkPipelineRenderingCreateInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.colorAttachmentCount = (uint32_t)desc.pipeline_rendering.color_formats.size();
    rendering_info.pColorAttachmentFormats = desc.pipeline_rendering.color_formats.data();
    rendering_info.depthAttachmentFormat = desc.pipeline_rendering.depth_format;
    rendering_info.stencilAttachmentFormat = desc.pipeline_rendering.stencil_format;
    pipeline_info.pNext = &rendering_info;

    VK_ASSERT(vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline->handle));
}

void ez_destroy_graphics_pipeline(EzGraphicsPipeline pipeline)
{
    res_mgr.destroyer_pipelines.emplace_back(pipeline->handle, ctx.frame_count);
    delete pipeline;
}

// Funcs
void ez_begin_rendering(const EzVkRenderingInfo& rendering_info)
{
    std::vector<VkRenderingAttachmentInfo> color_attachments;
    for (auto& x : rendering_info.colors)
    {
        VkRenderingAttachmentInfo color_attachment = {};
        color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment.imageView = x.texture->views[x.texture_view];
        color_attachment.imageLayout = x.texture->layout;
        color_attachment.loadOp = x.load_op;
        color_attachment.storeOp = x.store_op;
        color_attachment.clearValue = x.clear_value;
        color_attachments.push_back(color_attachment);
    }

    std::vector<VkRenderingAttachmentInfo> depth_attachments;
    for (auto& x : rendering_info.depth)
    {
        VkRenderingAttachmentInfo depth_attachment = {};
        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment.imageView = x.texture->views[x.texture_view];
        depth_attachment.imageLayout = x.texture->layout;
        depth_attachment.loadOp = x.load_op;
        depth_attachment.storeOp = x.store_op;
        depth_attachment.clearValue = x.clear_value;
        depth_attachments.push_back(depth_attachment);
        break;
    }

    VkRenderingInfo pass_info = {};
    pass_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    pass_info.renderArea.extent.width = rendering_info.width;
    pass_info.renderArea.extent.height = rendering_info.height;
    pass_info.layerCount = 1;
    pass_info.colorAttachmentCount = (uint32_t)color_attachments.size();
    pass_info.pColorAttachments = color_attachments.data();
    pass_info.pDepthAttachment = depth_attachments.data();

    vkCmdBeginRendering(ctx.cmd, &pass_info);
}

void ez_end_rendering()
{
    vkCmdEndRendering(ctx.cmd);
}

void ez_bind_scissor(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    VkRect2D scissor;
    scissor.extent.width = abs(right - left);
    scissor.extent.height = abs(top - bottom);
    scissor.offset.x = left;
    scissor.offset.y = top;
    vkCmdSetScissor(ctx.cmd, 0, 1, &scissor);
}

void ez_bind_viewport(float x, float y, float w, float h, float min_depth, float max_depth)
{
    VkViewport viewport;
    viewport.x = x;
    viewport.y = y;
    viewport.width = w;
    viewport.height = h;
    viewport.minDepth = min_depth;
    viewport.maxDepth = max_depth;
    vkCmdSetViewport(ctx.cmd, 0, 1, &viewport);
}

void ez_bind_vertex_buffer(EzBuffer vertex_buffer, uint64_t offset)
{
    vkCmdBindVertexBuffers(ctx.cmd, 0, 1, &vertex_buffer->handle, &offset);
}

void ez_bind_index_buffer(EzBuffer index_buffer, VkIndexType type, uint64_t offset)
{
    vkCmdBindIndexBuffer(ctx.cmd, index_buffer->handle, offset, type);
}

void ez_bind_srv(uint32_t slot, EzTexture texture, int texture_view)
{

}

void ez_bind_uav(uint32_t slot, EzTexture texture, int texture_view)
{

}

void ez_bind_cbv(uint32_t slot, EzBuffer buffer, uint64_t size, uint64_t offset)
{

}

void ez_bind_sampler(uint32_t slot, EzSampler sampler)
{

}

void ez_bind_graphics_pipeline(EzGraphicsPipeline pipeline)
{
    ctx.graphics_pipeline = pipeline;
    ctx.compute_pipeline = VK_NULL_HANDLE;
}

void ez_bind_compute_pipeline(EzComputePipeline pipeline)
{
    ctx.compute_pipeline = pipeline;
    ctx.graphics_pipeline = VK_NULL_HANDLE;
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

void ez_pipeline_barrier(VkDependencyFlags dependency_flags,
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

    vkCmdPipelineBarrier2(ctx.cmd, &dependency_info);
}