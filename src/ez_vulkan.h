#pragma once

#define VK_DEBUG
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <volk.h>
#include <stdio.h>
#include <stdlib.h>
#include <set>
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

void ez_flush();

// Swapchain
struct EzSwapchain_T
{
    uint32_t width;
    uint32_t height;
    uint32_t image_index;
    uint32_t image_count;
    VkAccessFlags2 access_mask = 0;
    VkPipelineStageFlags2 stage_mask = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
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
    VkAccessFlags2 access_mask = 0;
    VkPipelineStageFlags2 stage_mask = 0;
};
VK_DEFINE_HANDLE(EzBuffer)

struct EzBufferDesc
{
    size_t size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memory_flags;
};
void ez_create_buffer(const EzBufferDesc& desc, EzBuffer& buffer);

void ez_destroy_buffer(EzBuffer buffer);

void ez_map_memory(EzBuffer buffer, uint32_t size, uint32_t offset, void** memory_ptr);

void ez_unmap_memory(EzBuffer buffer);

void ez_copy_buffer(EzBuffer src_buffer, EzBuffer dst_buffer, VkBufferCopy range);

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
    VkAccessFlags2 access_mask = 0;
    VkPipelineStageFlags2 stage_mask = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
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
void ez_create_texture(const EzTextureDesc& desc, EzTexture& texture);

void ez_destroy_texture(EzTexture texture);

int ez_create_texture_view(EzTexture texture, VkImageViewType view_type,
                           uint32_t base_level, uint32_t level_count,
                           uint32_t base_layer, uint32_t layer_count);

void ez_copy_buffer_to_image(EzBuffer buffer, EzTexture texture, VkBufferImageCopy range);

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
void ez_create_sampler(const EzSamplerDesc& desc, EzSampler& sampler);

void ez_destroy_sampler(EzSampler sampler);

// Pipeline
struct EzShader_T
{
    VkShaderModule handle = VK_NULL_HANDLE;
    VkPipelineShaderStageCreateInfo stage_info = {};
    std::vector<VkPushConstantRange> pushconstants;
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
};
VK_DEFINE_HANDLE(EzShader)

void ez_create_shader(void* data, size_t size, EzShader& shader);

void ez_destroy_shader(EzShader shader);

struct EzPipeline_T
{
    VkPipelineBindPoint bind_point;
    VkPipeline handle = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
    std::vector<VkPushConstantRange> pushconstants;
};
VK_DEFINE_HANDLE(EzPipeline)

struct EzInputElement
{
    uint32_t binding = 0;
    uint32_t location = 0;
    uint32_t offset = 0;
    uint32_t size = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX;
};

struct EzInputAssembly
{
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
};

struct EzInputLayout
{
    std::vector<EzInputElement> elements;
};

struct EzBlendState
{
    bool blend_enable = false;
    VkBlendFactor src_color = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor dst_color = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    VkBlendOp color_op = VK_BLEND_OP_ADD;
    VkBlendFactor src_alpha = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dst_alpha = VK_BLEND_FACTOR_ONE;
    VkBlendOp alpha_op = VK_BLEND_OP_ADD;
};

struct EzRasterizationState
{
    VkPolygonMode fill_mode = VK_POLYGON_MODE_FILL;
    VkFrontFace front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkCullModeFlagBits cull_mode = VK_CULL_MODE_NONE;
};

struct EzMultisampleState
{
    bool multi_sample_enable = false;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
};

struct EzDepthStencilOp
{
    VkStencilOp stencil_fail_op = VK_STENCIL_OP_KEEP;
    VkStencilOp stencil_depth_fail_op = VK_STENCIL_OP_KEEP;
    VkStencilOp stencil_pass_op = VK_STENCIL_OP_KEEP;
    VkCompareOp stencil_func = VK_COMPARE_OP_ALWAYS;
};

struct EzDepthStencilState
{
    bool depth_test = false;
    bool depth_write = true;
    VkCompareOp depth_func = VK_COMPARE_OP_LESS_OR_EQUAL;
    bool stencil_test = false;
    uint8_t stencil_read_mask = 0xff;
    uint8_t stencil_write_mask = 0xff;
    EzDepthStencilOp front_face = {};
    EzDepthStencilOp back_face = {};
};

struct EzPipelineRendering
{
    std::vector<VkFormat> color_formats;
    VkFormat depth_format = VK_FORMAT_UNDEFINED;
    VkFormat stencil_format = VK_FORMAT_UNDEFINED;
};

struct EzGraphicsPipelineDesc
{
    EzShader vertex_shader = VK_NULL_HANDLE;
    EzShader fragment_shader = VK_NULL_HANDLE;
    EzInputAssembly input_assembly = {};
    EzInputLayout input_layout = {};
    EzBlendState blend_state = {};
    EzRasterizationState rasterization_state = {};
    EzMultisampleState multisample_state = {};
    EzPipelineRendering pipeline_rendering = {};
};
void ez_create_graphics_pipeline(const EzGraphicsPipelineDesc& desc, EzPipeline& pipeline);

void ez_destroy_graphics_pipeline(EzPipeline pipeline);

void ez_create_compute_pipeline(EzShader shader, EzPipeline& pipeline);

void ez_destroy_compute_pipeline(EzPipeline pipeline);

// Funcs
struct EzRenderingAttachmentInfo
{
    EzTexture texture;
    int texture_view;
    VkAttachmentLoadOp load_op;
    VkAttachmentStoreOp store_op;
    VkClearValue clear_value;
};

struct EzVkRenderingInfo
{
    uint32_t width;
    uint32_t height;
    std::vector<EzRenderingAttachmentInfo> depth;
    std::vector<EzRenderingAttachmentInfo> colors;
};
void ez_begin_rendering(const EzVkRenderingInfo& rendering_info);

void ez_end_rendering();

void ez_bind_scissor(int32_t left, int32_t top, int32_t right, int32_t bottom);

void ez_bind_viewport(float x, float y, float w, float h, float min_depth = 0.0f, float max_depth = 1.0f);

void ez_bind_vertex_buffer(EzBuffer vertex_buffer, uint64_t offset = 0);

void ez_bind_index_buffer(EzBuffer index_buffer, VkIndexType type, uint64_t offset = 0);

void ez_bind_graphics_pipeline(EzPipeline pipeline);

void ez_bind_compute_pipeline(EzPipeline pipeline);

void ez_bind_descriptor(uint32_t binding, EzTexture texture, int texture_view);

void ez_bind_descriptor(uint32_t binding, EzBuffer buffer, uint64_t size, uint64_t offset = 0);

void ez_bind_descriptor(uint32_t binding, EzSampler sampler);

void ez_push_constants(VkShaderStageFlags stages, const void* data, uint32_t size);

void ez_draw(uint32_t vertex_count, uint32_t vertex_offset);

void ez_draw_indexed(uint32_t index_count, uint32_t index_offset, int32_t vertex_offset);

void ez_draw_instanced(uint32_t vertex_count, uint32_t instance_count, uint32_t vertex_offset, uint32_t instance_offset);

void ez_dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z);

// Barrier
VkImageMemoryBarrier2 ez_image_barrier(EzSwapchain swapchain,
                                       VkPipelineStageFlags2 stage_mask,
                                       VkAccessFlags2 access_mask,
                                       VkImageLayout layout,
                                       VkImageAspectFlags aspect_mask);

VkImageMemoryBarrier2 ez_image_barrier(EzTexture texture,
                                       VkPipelineStageFlags2 stage_mask,
                                       VkAccessFlags2 access_mask,
                                       VkImageLayout layout,
                                       VkImageAspectFlags aspect_mask);

VkBufferMemoryBarrier2 ez_buffer_barrier(EzBuffer buffer,
                                        VkPipelineStageFlags2 stage_mask,
                                        VkAccessFlags2 access_mask);

void ez_pipeline_barrier(VkDependencyFlags dependency_flags,
                         size_t buffer_barrier_count,
                         const VkBufferMemoryBarrier2* buffer_barriers,
                         size_t image_barrier_count,
                         const VkImageMemoryBarrier2* image_barriers);