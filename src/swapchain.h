#pragma once

#include "context.h"
#include <vector>

struct Swapchain
{
    uint32_t width;
    uint32_t height;
    uint32_t image_index;
    uint32_t image_count;
    VkSwapchainKHR handle;
    VkSurfaceKHR surface;
    VkSemaphore acquire_semaphore = VK_NULL_HANDLE;
    VkSemaphore release_semaphore = VK_NULL_HANDLE;
    std::vector<VkImage> images;
};

enum class SwapchainStatus
{
    Ready,
    Resized,
    NotReady,
};

void create_swapchain(Context& ctx, Swapchain& swapchain, void* window);

void destroy_swapchain(Context& ctx, Swapchain& swapchain);

SwapchainStatus update_swapchain(Context& ctx, Swapchain& result);

void acquire_next_image(Context& ctx, Swapchain& swapchain);

void present(Context& ctx, Swapchain& swapchain);