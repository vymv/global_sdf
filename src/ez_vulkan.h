#pragma once

#define VK_DEBUG
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <volk.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#define VK_LOGE(...)                                 \
    do                                               \
    {                                                \
        fprintf(stderr, "[VK_ERROR]: " __VA_ARGS__); \
        fflush(stderr);                              \
    } while (false)

#define VK_LOGW(...)                                \
    do                                              \
    {                                               \
        fprintf(stderr, "[VK_WARN]: " __VA_ARGS__); \
        fflush(stderr);                             \
    } while (false)

#define VK_LOGI(...)                                \
    do                                              \
    {                                               \
        fprintf(stderr, "[VK_INFO]: " __VA_ARGS__); \
        fflush(stderr);                             \
    } while (false)

#define VK_ASSERT(x)                                                 \
    do                                                               \
    {                                                                \
        if (x != VK_SUCCESS)                                         \
        {                                                            \
            VK_LOGE("Vulkan error at %s:%d.\n", __FILE__, __LINE__); \
            abort();                                                 \
        }                                                            \
    } while (0)

inline constexpr uint32_t ez_align_to(uint32_t value, uint32_t alignment)
{
    return ((value + alignment - 1) / alignment) * alignment;
}

inline constexpr uint64_t ez_align_to(uint64_t value, uint64_t alignment)
{
    return ((value + alignment - 1) / alignment) * alignment;
}

// Core
void ez_init();

void ez_terminate();

void ez_submit();

VkCommandBuffer ez_cmd();

VkDevice ez_device();

void ez_wait_idle();

// Swapchain
struct EzSwapchain_T
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
VK_DEFINE_HANDLE(EzSwapchain)

enum class EzSwapchainStatus
{
    Ready,
    Resized,
    NotReady,
};

void ez_create_swapchain(void* window, EzSwapchain& swapchain);

void ez_destroy_swapchain(EzSwapchain swapchain);

EzSwapchainStatus ez_update_swapchain(EzSwapchain swapchain);

void ez_acquire_next_image(EzSwapchain swapchain);

void ez_present(EzSwapchain swapchain);

// Buffer
struct EzBuffer_T
{
    size_t size;
    VkBuffer handle;
    VkDeviceMemory memory;
};
VK_DEFINE_HANDLE(EzBuffer)

void ez_create_buffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_flags, EzBuffer& buffer);

void ez_destroy_buffer(EzBuffer buffer);

struct EzAllocation
{
    uint64_t offset = 0;
    EzBuffer buffer = VK_NULL_HANDLE;
};
EzAllocation ez_alloc_stage_buffer(size_t size);

// Barrier
VkImageMemoryBarrier2 ez_image_barrier(VkImage image,
                                    VkPipelineStageFlags2 src_stage_mask, VkAccessFlags2 src_access_mask, VkImageLayout old_layout,
                                    VkPipelineStageFlags2 dst_stage_mask, VkAccessFlags2 dst_access_mask, VkImageLayout new_layout,
                                    VkImageAspectFlags aspect_mask);

VkBufferMemoryBarrier2 ez_buffer_barrier(VkBuffer buffer,
                                      VkPipelineStageFlags2 src_stage_mask, VkAccessFlags2 src_access_mask,
                                      VkPipelineStageFlags2 dst_stage_mask, VkAccessFlags2 dst_access_mask);

void ez_pipeline_barrier(VkCommandBuffer cmd, VkDependencyFlags dependency_flags,
                      size_t buffer_barrier_count, const VkBufferMemoryBarrier2* buffer_barriers,
                      size_t image_barrier_count, const VkImageMemoryBarrier2* image_barriers);