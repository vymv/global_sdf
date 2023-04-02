#include "sdf_generator.h"
#include <math/detection.h>

SDF* generate_sdf(const BoundingBox& bounds, uint32_t resolution, uint32_t vertex_count, float* vertices, uint32_t index_count, uint32_t* indices)
{
    SDF* sdf = new SDF();
    float max_distance = bounds.get_max_extent();
    glm::vec3 bounds_size = bounds.get_size();
    glm::vec3 uvw_to_local_mul = bounds_size;
    glm::vec3 uvw_to_local_add = bounds.bb_min;
    glm::vec3 xyz_to_local_mul = uvw_to_local_mul / glm::vec3((float)(resolution - 1));
    glm::vec3 xyz_to_local_add = uvw_to_local_add;
    sdf->max_distance = max_distance;
    sdf->uvw_to_local_mul = uvw_to_local_mul;
    sdf->uvw_to_local_add = uvw_to_local_add;

    uint32_t voxel_data_size = resolution * resolution * resolution * sizeof(float);
    float* voxel_data = (float*)malloc(voxel_data_size);
    uint32_t sample_count = 6;
    glm::vec3 sample_directions[6] = {
        glm::vec3(0.0f, 1.0f, 0.0f),    // Up
        glm::vec3(0.0f, -1.0f, 0.0f),   // Down
        glm::vec3(-1.0f, 0.0f, 0.0f),   // Left
        glm::vec3(1.0f, 0.0f, 0.0f),    // Right
        glm::vec3(0.0f, 0.0f, 1.0f),    // Forward
        glm::vec3(0.0f, 0.0f, -1.0f)    // Backward
    };

    for (int x = 0; x < resolution; ++x)
    {
        for (int y = 0; y < resolution; ++y)
        {
            for (int z = 0; z < resolution; ++z)
            {
                glm::vec3 voxel_pos = glm::vec3((float)x, (float)y, (float)z) * xyz_to_local_mul + xyz_to_local_add;
                float min_distance = max_distance;
                bool hit = false;
                float hit_distance = INF;
                glm::vec3 hit_position;
                glm::vec3 hit_normal;
                for (int i = 0; i < index_count; i += 3)
                {
                    glm::vec3 v0(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
                    glm::vec3 v1(vertices[i * 6], vertices[i * 6 + 1], vertices[i * 6 + 2]);
                    glm::vec3 v2(vertices[i * 9], vertices[i * 9 + 1], vertices[i * 9 + 2]);
                    glm::vec3 p = closest_point_on_triangle(voxel_pos, v0, v1, v2);
                    float distance = glm::distance(p, voxel_pos);
                    if (hit_distance >= distance)
                    {
                        hit = true;
                        hit_distance = distance;
                        hit_position = p;
                    }
                }
                if (hit)
                {
                    min_distance = hit_distance;
                }

                uint32_t hit_back_count = 0, hit_count = 0;
                for (int sample = 0; sample < sample_count; sample++)
                {
                    hit = false;
                    hit_distance = INF;
                    glm::vec3 ro = voxel_pos;
                    glm::vec3 rd = sample_directions[sample];
                    for (int i = 0; i < index_count; i += 3)
                    {
                        glm::vec3 v0(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
                        glm::vec3 v1(vertices[i * 6], vertices[i * 6 + 1], vertices[i * 6 + 2]);
                        glm::vec3 v2(vertices[i * 9], vertices[i * 9 + 1], vertices[i * 9 + 2]);
                        if (ray_intersect_triangle(ro, rd, v0, v1, v2, hit_distance))
                        {
                            hit = true;
                            hit_normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                        }
                    }
                    if (hit)
                    {
                        hit_count++;
                        if (glm::dot(rd, hit_normal) > 0)
                            hit_back_count++;
                    }
                }

                if ((float)hit_back_count > (float)sample_count * 0.5f && hit_count != 0)
                {
                    min_distance *= -1;
                }
                uint32_t z_address = resolution * resolution * z;
                uint32_t y_address = resolution * y + z_address;
                uint32_t x_address = x + y_address;
                *(voxel_data + x_address) = min_distance;
            }
        }
    }

    EzTextureDesc texture_desc{};
    texture_desc.width = resolution;
    texture_desc.height = resolution;
    texture_desc.depth = resolution;
    texture_desc.format = VK_FORMAT_R32_SFLOAT;
    texture_desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    ez_create_texture(texture_desc, sdf->texture);
    ez_create_texture_view(sdf->texture, VK_IMAGE_VIEW_TYPE_3D, 0, 1, 0, 1);

    VkImageMemoryBarrier2 barrier = ez_image_barrier(sdf->texture, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    ez_pipeline_barrier(0, 0, nullptr, 1, &barrier);

    EzStageAllocation alloc = ez_alloc_stage_buffer(voxel_data_size);
    VkBufferImageCopy range{};
    range.bufferOffset = alloc.offset;
    range.imageExtent.width = resolution;
    range.imageExtent.height = resolution;
    range.imageExtent.depth = resolution;
    range.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.imageSubresource.mipLevel = 0;
    range.imageSubresource.baseArrayLayer = 0;
    range.imageSubresource.layerCount = 1;
    ez_copy_buffer_to_image(alloc.buffer, sdf->texture, range);

    barrier = ez_image_barrier(sdf->texture, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    ez_pipeline_barrier(0, 0, nullptr, 1, &barrier);
    free(voxel_data);
    return sdf;
}