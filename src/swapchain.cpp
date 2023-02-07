#include "swapchain.h"
#ifdef WIN32
#include <windows.h>
#endif

void create_swapchain(Context& ctx, Swapchain& swapchain, void* window)
{
#ifdef WIN32
    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.pNext = nullptr;
    surface_create_info.flags = 0;
    surface_create_info.hinstance = ::GetModuleHandle(nullptr);
    surface_create_info.hwnd = (HWND)window;
    VK_ASSERT(vkCreateWin32SurfaceKHR(ctx.instance, &surface_create_info, nullptr, &swapchain.surface));
#endif

    VkSurfaceCapabilitiesKHR surface_caps;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physical_device, swapchain.surface, &surface_caps));
    swapchain.width = surface_caps.currentExtent.width;
    swapchain.height = surface_caps.currentExtent.height;

    VkSwapchainCreateInfoKHR sc_create_info = {};
    sc_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sc_create_info.surface = swapchain.surface;
    sc_create_info.minImageCount = 2;
    sc_create_info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    sc_create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sc_create_info.imageExtent.width = swapchain.width;
    sc_create_info.imageExtent.height = swapchain.height;
    sc_create_info.imageArrayLayers = 1;
    sc_create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    sc_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    sc_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sc_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sc_create_info.oldSwapchain = VK_NULL_HANDLE;

    VK_ASSERT(vkCreateSwapchainKHR(ctx.device, &sc_create_info, nullptr, &swapchain.handle));

    VK_ASSERT(vkGetSwapchainImagesKHR(ctx.device, swapchain.handle, &swapchain.image_count, 0));
    swapchain.images.resize(swapchain.image_count);
    VK_ASSERT(vkGetSwapchainImagesKHR(ctx.device, swapchain.handle, &swapchain.image_count, swapchain.images.data()));

    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_ASSERT(vkCreateSemaphore(ctx.device, &semaphore_create_info, nullptr, &swapchain.acquire_semaphore));
    VK_ASSERT(vkCreateSemaphore(ctx.device, &semaphore_create_info, nullptr, &swapchain.release_semaphore));
}

void destroy_swapchain(Context& ctx, Swapchain& swapchain)
{
    vkDestroySwapchainKHR(ctx.device, swapchain.handle, nullptr);
    vkDestroySurfaceKHR(ctx.instance, swapchain.surface, nullptr);
    vkDestroySemaphore(ctx.device, swapchain.acquire_semaphore, nullptr);
    vkDestroySemaphore(ctx.device, swapchain.release_semaphore, nullptr);
}

SwapchainStatus update_swapchain(Context& ctx, Swapchain& swapchain)
{
    VkSurfaceCapabilitiesKHR surface_caps;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physical_device, swapchain.surface, &surface_caps));
    uint32_t new_width = surface_caps.currentExtent.width;
    uint32_t new_height = surface_caps.currentExtent.height;

    if (new_width == 0 || new_height == 0)
        return SwapchainStatus::NotReady;

    if (swapchain.width == new_width && swapchain.height == new_height)
        return SwapchainStatus::Ready;

    swapchain.width = new_width;
    swapchain.height = new_height;
    VkSwapchainKHR old_handle = swapchain.handle;
    VkSwapchainCreateInfoKHR sc_create_info = {};
    sc_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sc_create_info.surface = swapchain.surface;
    sc_create_info.minImageCount = 2;
    sc_create_info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    sc_create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sc_create_info.imageExtent.width = swapchain.width;
    sc_create_info.imageExtent.height = swapchain.height;
    sc_create_info.imageArrayLayers = 1;
    sc_create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    sc_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    sc_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sc_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sc_create_info.oldSwapchain = old_handle;

    VK_ASSERT(vkCreateSwapchainKHR(ctx.device, &sc_create_info, nullptr, &swapchain.handle));

    VK_ASSERT(vkGetSwapchainImagesKHR(ctx.device, swapchain.handle, &swapchain.image_count, 0));
    swapchain.images.resize(swapchain.image_count);
    VK_ASSERT(vkGetSwapchainImagesKHR(ctx.device, swapchain.handle, &swapchain.image_count, swapchain.images.data()));

    vkDestroySwapchainKHR(ctx.device, old_handle, nullptr);

    return SwapchainStatus::Resized;
}

void acquire_next_image(Context& ctx, Swapchain& swapchain)
{
    vkAcquireNextImageKHR(ctx.device, swapchain.handle, ~0ull, swapchain.acquire_semaphore, VK_NULL_HANDLE, &swapchain.image_index);
    ctx.acquire_semaphore = swapchain.acquire_semaphore;
}

void present(Context& ctx, Swapchain& swapchain)
{
    ctx.swapchain = swapchain.handle;
    ctx.release_semaphore = swapchain.release_semaphore;
}