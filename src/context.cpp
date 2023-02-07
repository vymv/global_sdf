#include "context.h"
#include <vector>

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

void create_context(Context& ctx)
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
    features_1_1.pNext = &features_1_2;
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

void destroy_context(Context& ctx)
{
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

void submit(Context& ctx)
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

    ctx.swapchain = VK_NULL_HANDLE;
    ctx.acquire_semaphore = VK_NULL_HANDLE;
    ctx.release_semaphore = VK_NULL_HANDLE;
}