#include "sdf_generator.h"
#include <math/detection.h>

SDF* generate_sdf(const BoundingBox& bounds, uint32_t resolution, uint32_t vertex_count, float* vertices, uint32_t index_count, uint8_t* indices, VkIndexType index_type)
{
    SDF* sdf = new SDF();
    sdf->bounds = bounds;
    sdf->resolution = resolution;
    float max_distance = bounds.get_max_extent();
    glm::vec3 bounds_size = bounds.get_size();
    glm::vec3 uvw_to_local_mul = bounds_size;
    glm::vec3 uvw_to_local_add = bounds.bb_min;
    glm::vec3 xyz_to_local_mul = uvw_to_local_mul / glm::vec3((float)(resolution - 1));
    glm::vec3 xyz_to_local_add = uvw_to_local_add;
    sdf->local_to_uvw_mul = 1.0f / uvw_to_local_mul;
    sdf->local_to_uvw_add = -uvw_to_local_add / uvw_to_local_mul;
    uint32_t voxel_data_size = resolution * resolution * resolution * sizeof(float);
    float* voxel_data = (float*)malloc(voxel_data_size);
    uint32_t sample_count = 6;
    glm::vec3 sample_directions[6] = {
        glm::vec3(0.0f, 1.0f, 0.0f), // Up
        glm::vec3(0.0f, -1.0f, 0.0f),// Down
        glm::vec3(-1.0f, 0.0f, 0.0f),// Left
        glm::vec3(1.0f, 0.0f, 0.0f), // Right
        glm::vec3(0.0f, 0.0f, 1.0f), // Forward
        glm::vec3(0.0f, 0.0f, -1.0f) // Backward
    };

    uint32_t* indices_32;
    uint16_t* indices_16;
    if (index_type == VK_INDEX_TYPE_UINT32)
    {
        indices_32 = (uint32_t*)indices;
    }
    else
    {
        indices_16 = (uint16_t*)indices;
    }

#pragma omp parallel for
    for (int x = 0; x < resolution; ++x)
    {
#pragma omp parallel for
        for (int y = 0; y < resolution; ++y)
        {
#pragma omp parallel for
            for (int z = 0; z < resolution; ++z)
            {
                glm::vec3 voxel_pos = glm::vec3((float)x, (float)y, (float)z) * xyz_to_local_mul + xyz_to_local_add;
                float min_distance = max_distance;
                bool hit = false;
                float hit_distance = INF;
                glm::vec3 hit_position;
                glm::vec3 hit_normal;
                int idx0;
                int idx1;
                int idx2;
                // 对这个mesh中的每个三角面片
                for (int i = 0; i < index_count; i += 3)
                {
                    if (index_type == VK_INDEX_TYPE_UINT32)
                    {
                        idx0 = (int)indices_32[i];
                        idx1 = (int)indices_32[i + 1];
                        idx2 = (int)indices_32[i + 2];
                    }
                    else
                    {
                        idx0 = (int)indices_16[i];
                        idx1 = (int)indices_16[i + 1];
                        idx2 = (int)indices_16[i + 2];
                    }

                    glm::vec3 v0(vertices[idx0 * 3], vertices[idx0 * 3 + 1], vertices[idx0 * 3 + 2]);
                    glm::vec3 v1(vertices[idx1 * 3], vertices[idx1 * 3 + 1], vertices[idx1 * 3 + 2]);
                    glm::vec3 v2(vertices[idx2 * 3], vertices[idx2 * 3 + 1], vertices[idx2 * 3 + 2]);
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
                    min_distance = hit_distance;// 找到离voxel_pos最近的三角面片，计算距离
                }

                uint32_t hit_back_count = 0, hit_count = 0;

                // 朝六个方向trace ray，判断sdf符号
                for (int sample = 0; sample < sample_count; sample++)
                {
                    hit = false;
                    hit_distance = INF;
                    glm::vec3 ro = voxel_pos;
                    glm::vec3 rd = sample_directions[sample];

                    for (int i = 0; i < index_count; i += 3)
                    {
                        if (index_type == VK_INDEX_TYPE_UINT32)
                        {
                            idx0 = (int)indices_32[i];
                            idx1 = (int)indices_32[i + 1];
                            idx2 = (int)indices_32[i + 2];
                        }
                        else
                        {
                            idx0 = (int)indices_16[i];
                            idx1 = (int)indices_16[i + 1];
                            idx2 = (int)indices_16[i + 2];
                        }

                        glm::vec3 v0(vertices[idx0 * 3], vertices[idx0 * 3 + 1], vertices[idx0 * 3 + 2]);
                        glm::vec3 v1(vertices[idx1 * 3], vertices[idx1 * 3 + 1], vertices[idx1 * 3 + 2]);
                        glm::vec3 v2(vertices[idx2 * 3], vertices[idx2 * 3 + 1], vertices[idx2 * 3 + 2]);

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

                if ((float)hit_back_count > (float)sample_count * 0.2f && hit_count != 0)
                {
                    min_distance *= -1;
                }
                uint32_t z_address = resolution * resolution * z;
                uint32_t y_address = resolution * y + z_address;
                uint32_t x_address = x + y_address;
                *(voxel_data + x_address) = min_distance;// 写入结果到voxel_data
            }
        }
    }

    EzTextureDesc texture_desc{};
    texture_desc.width = resolution;
    texture_desc.height = resolution;
    texture_desc.depth = resolution;
    texture_desc.format = VK_FORMAT_R32_SFLOAT;
    texture_desc.image_type = VK_IMAGE_TYPE_3D;
    texture_desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    ez_create_texture(texture_desc, sdf->texture);
    ez_create_texture_view(sdf->texture, VK_IMAGE_VIEW_TYPE_3D, 0, 1, 0, 1);

    VkImageMemoryBarrier2 barrier = ez_image_barrier(sdf->texture, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    ez_pipeline_barrier(0, 0, nullptr, 1, &barrier);

    VkBufferImageCopy range{};
    range.imageExtent.width = resolution;
    range.imageExtent.height = resolution;
    range.imageExtent.depth = resolution;
    range.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.imageSubresource.mipLevel = 0;
    range.imageSubresource.baseArrayLayer = 0;
    range.imageSubresource.layerCount = 1;
    ez_update_image(sdf->texture, range, voxel_data);// 最终结果写入到sdf->texture

    barrier = ez_image_barrier(sdf->texture, EZ_RESOURCE_STATE_SHADER_RESOURCE);
    ez_pipeline_barrier(0, 0, nullptr, 1, &barrier);
    free(voxel_data);
    return sdf;
}