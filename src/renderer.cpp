#include "renderer.h"

void upload_uniform_buffer(EzBuffer buffer, uint32_t size, uint32_t offset, void* data)
{
    VkBufferMemoryBarrier2 barrier = ez_buffer_barrier(buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);

    ez_update_buffer(buffer, size, offset, data);

    VkPipelineStageFlags2 stage_flags = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    VkAccessFlags2 access_flags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    barrier = ez_buffer_barrier(buffer, stage_flags, access_flags);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);
}