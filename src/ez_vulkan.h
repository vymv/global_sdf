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

struct EzBufferDesc
{
    size_t size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memory_flags;
};
void ez_create_buffer(EzBufferDesc desc, EzBuffer& buffer);

void ez_destroy_buffer(EzBuffer buffer);

struct EzAllocation
{
    uint64_t offset = 0;
    EzBuffer buffer = VK_NULL_HANDLE;
};
EzAllocation ez_alloc_stage_buffer(size_t size);

// Texture
struct EzTexture_T
{
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t levels;
    uint32_t layers;
    VkFormat format;
    VkImage handle;
    VkDeviceMemory memory;
    std::vector<VkImageView> views;
};
VK_DEFINE_HANDLE(EzTexture)

struct EzTextureDesc
{
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t levels;
    uint32_t layers;
    VkFormat format;
    VkImageType image_type;
    VkImageUsageFlags usage;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
};
void ez_create_texture(EzTextureDesc desc, EzTexture& texture);

void ez_destroy_texture(EzTexture texture);

int ez_create_texture_view(EzTexture texture, VkImageViewType view_type,
                           uint32_t base_level, uint32_t level_count,
                           uint32_t base_layer, uint32_t layer_count);

struct EzSampler_T
{
    VkSampler handle = VK_NULL_HANDLE;
};
VK_DEFINE_HANDLE(EzSampler)

struct EzSamplerDesc
{
    VkFilter mag_filter = VK_FILTER_LINEAR;
    VkFilter min_filter = VK_FILTER_LINEAR;
    VkSamplerAddressMode address_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode address_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode address_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkBorderColor border_color = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    bool anisotropy_enable = false;
};
void ez_create_sampler(EzSamplerDesc desc, EzSampler& sampler);

void ez_destroy_sampler(EzSampler sampler);

// Pipeline
struct EzShader_T
{
    VkShaderModule handle = VK_NULL_HANDLE;
    VkPipelineShaderStageCreateInfo stage_info = {};
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
};
VK_DEFINE_HANDLE(EzShader)

void ez_create_shader(void* data, size_t size, EzShader& shader);

void ez_destroy_shader(EzShader shader);

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