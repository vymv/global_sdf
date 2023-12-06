#include "sdf_generator.h"
#include "glm/detail/qualifier.hpp"
#include "glm/ext/scalar_constants.hpp"
#include <algorithm>
#include <execution>
#include <math/detection.h>

#include <fstream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

glm::vec3 closest_point_on_triangle_m(const glm::vec3& p, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
    glm::vec3 edge0 = v1 - v0;
    glm::vec3 edge1 = v2 - v0;
    glm::vec3 v0p = p - v0;

    float dot00 = glm::dot(edge0, edge0);
    float dot01 = glm::dot(edge0, edge1);
    float dot11 = glm::dot(edge1, edge1);
    float dot0p = glm::dot(edge0, v0p);
    float dot1p = glm::dot(edge1, v0p);

    float denom = dot00 * dot11 - dot01 * dot01;

    float u = (dot11 * dot0p - dot01 * dot1p) / denom;
    float v = (dot00 * dot1p - dot01 * dot0p) / denom;

    if (u >= 0 && v >= 0 && u + v <= 1)
    {
        return v0 + u * edge0 + v * edge1;
    }
    else
    {
        // If the closest point is outside the triangle, return the closest vertex
        float dist0 = glm::distance(p, v0);
        float dist1 = glm::distance(p, v1);
        float dist2 = glm::distance(p, v2);

        if (dist0 <= dist1 && dist0 <= dist2)
        {
            return v0;
        }
        else if (dist1 <= dist0 && dist1 <= dist2)
        {
            return v1;
        }
        else
        {
            return v2;
        }
    }
}
glm::vec3 calculateBarycentricCoordinates(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, const glm::vec3& P)
{
    glm::vec3 v0 = B - A;
    glm::vec3 v1 = C - A;
    glm::vec3 v2 = P - A;

    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);

    float denom = d00 * d11 - d01 * d01;

    // Check if the denominator is close to zero
    if (glm::abs(denom) < glm::epsilon<float>())
    {
        // Handle division by zero error, return a default value or take appropriate action
        // std::cerr << "Error: Denominator is zero." << std::endl;
        return glm::vec3(0.0f);
    }

    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return glm::vec3(u, v, w);
}
glm::vec3 interpolateNormals(const glm::vec3& normalA, const glm::vec3& normalB, const glm::vec3& normalC, const glm::vec3& barycentricCoords)
{
    return barycentricCoords.x * normalA + barycentricCoords.y * normalB + barycentricCoords.z * normalC;
}
namespace glm
{
using json = nlohmann::json;
void to_json(json& j, const ::glm::vec3& v)
{
    j = json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}

void from_json(const json& j, ::glm::vec3& v)
{
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
    j.at("z").get_to(v.z);
}
}// namespace glm

void to_json(json& j, const BoundingBox& bb)
{
    j = json{{"bb_min", bb.bb_min}, {"bb_max", bb.bb_max}};
}

void from_json(const json& j, BoundingBox& bb)
{
    j.at("bb_min").get_to(bb.bb_min);
    j.at("bb_max").get_to(bb.bb_max);
}

void to_json(json& j, const SDF& sdf)
{
    j = json{{"bounds", sdf.bounds}, {"resolution", sdf.resolution}, {"local_to_uvw_mul", sdf.local_to_uvw_mul}, {"local_to_uvw_add", sdf.local_to_uvw_add}};
}

void from_json(const json& j, SDF& sdf)
{
    j.at("bounds").get_to(sdf.bounds);
    j.at("resolution").get_to(sdf.resolution);
    j.at("local_to_uvw_mul").get_to(sdf.local_to_uvw_mul);
    j.at("local_to_uvw_add").get_to(sdf.local_to_uvw_add);
}

void to_json(json& j, const SDFCache& sdf)
{
    j = json{{"bounds", sdf.bounds}, {"resolution", sdf.resolution}, {"local_to_uvw_mul", sdf.local_to_uvw_mul}, {"local_to_uvw_add", sdf.local_to_uvw_add}, {"voxel_data", sdf.voxel_data}};
}

void from_json(const json& j, SDFCache& sdf)
{
    j.at("bounds").get_to(sdf.bounds);
    j.at("resolution").get_to(sdf.resolution);
    j.at("local_to_uvw_mul").get_to(sdf.local_to_uvw_mul);
    j.at("local_to_uvw_add").get_to(sdf.local_to_uvw_add);
    j.at("voxel_data").get_to(sdf.voxel_data);
}

SDF* generate_sdf(const BoundingBox& bounds, uint32_t resolution, uint32_t vertex_count, float* vertices, float* normal_data, uint32_t index_count, uint8_t* indices, VkIndexType index_type, const std::string& file_path)
{

    SDF* sdf;

#if USE_SDF_CACHE
    sdf = load_sdf_cached(file_path.c_str(), bounds, resolution);
    if (sdf)
    {
        return sdf;
    }
#endif
    sdf = new SDF();
    sdf->bounds = bounds;
    sdf->resolution = resolution;
    float max_distance = bounds.get_max_extent();
    glm::vec3 bounds_size = bounds.get_size();
    glm::vec3 uvw_to_local_mul = bounds_size;
    glm::vec3 uvw_to_local_add = bounds.bb_min;
    glm::vec3 xyz_to_local_mul = uvw_to_local_mul / glm::vec3((float)(resolution));
    glm::vec3 xyz_to_local_add = uvw_to_local_add;
    sdf->local_to_uvw_mul = 1.0f / uvw_to_local_mul;
    sdf->local_to_uvw_add = -uvw_to_local_add / uvw_to_local_mul;
    uint32_t voxel_data_size = resolution * resolution * resolution * sizeof(float);
    std::vector<float> voxel_data;
    voxel_data.resize(voxel_data_size / sizeof(float));
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
    std::for_each(std::execution::par, voxel_data.data(), voxel_data.data() + resolution * resolution * resolution, [&](float& v)
                  {
                      int index = &v - voxel_data.data();
                      int z = index % resolution;
                      int y = (index / resolution) % resolution;
                      int x = index / (resolution * resolution);

                      glm::vec3 voxel_pos = glm::vec3((float)x, (float)y, (float)z) * xyz_to_local_mul + xyz_to_local_add;
                      float min_distance = max_distance;
                      bool hit = false;
                      float hit_distance = INF;
                      glm::vec3 hit_position;
                      int idx0;
                      int idx1;
                      int idx2;

                      glm::vec3 closeset_point;
                      glm::vec3 hit_normal;
                      int hit_index = -1;

                      //   uint32_t hit_back_count = 0, hit_count = 0;
                      bool hit_back = false;
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

                          glm::vec3 n0;
                          glm::vec3 n1;
                          glm::vec3 n2;
                          if (normal_data == nullptr)
                          {
                              n0 = n1 = n2 = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                          }
                          else
                          {
                              n0 = glm::vec3(normal_data[idx0 * 3], normal_data[idx0 * 3 + 1], normal_data[idx0 * 3 + 2]);
                              n1 = glm::vec3(normal_data[idx1 * 3], normal_data[idx1 * 3 + 1], normal_data[idx1 * 3 + 2]);
                              n2 = glm::vec3(normal_data[idx2 * 3], normal_data[idx2 * 3 + 1], normal_data[idx2 * 3 + 2]);
                          }

                          glm::vec3 p = closest_point_on_triangle_m(voxel_pos, v0, v1, v2);
                          float distance = glm::distance(p, voxel_pos);
                          if (hit_distance >= distance)
                          {
                              hit = true;
                              hit_index = i;
                              hit_distance = distance;
                              hit_position = p;
                              closeset_point = p;
                              //   hit_normal = interpolateNormals(n0, n1, n2, calculateBarycentricCoordinates(v0, v1, v2, p));
                              //   hit_back = glm::dot(hit_normal, voxel_pos - p) < 0;
                          }
                      }
                      if (hit)
                      {
                          min_distance = hit_distance;// 找到离voxel_pos最近的三角面片，计算距离
                                                      //   if (x == 63 && y == 63 && z == 63)
                                                      //   {
                                                      //       int kk = 0;
                                                      //   }
                                                      //   if (hit_back)
                                                      //   {
                                                      //   min_distance *= -1;
                                                      //   }
                      }

                      uint32_t z_address = resolution * resolution * z;
                      uint32_t y_address = resolution * y + z_address;
                      uint32_t x_address = x + y_address;
                      *(&voxel_data[0] + x_address) = min_distance;// 写入结果到voxel_data
                  });

#if USE_SDF_CACHE
    save_sdf_cached(file_path.c_str(), sdf, voxel_data);
#endif
    uint32_t size = voxel_data.size();
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
    ez_update_image(sdf->texture, range, voxel_data.data());// 最终结果写入到sdf->texture

    barrier = ez_image_barrier(sdf->texture, EZ_RESOURCE_STATE_SHADER_RESOURCE);
    ez_pipeline_barrier(0, 0, nullptr, 1, &barrier);

    voxel_data.clear();
    return sdf;
}

SDF* load_sdf_cached(const char* path, const BoundingBox& bounds, uint32_t resolution)
{
    auto check_validate = [](const SDFCache& cache, const BoundingBox& bounds, uint32_t resolution) -> bool
    {
        if (cache.bounds.bb_max != bounds.bb_max || cache.bounds.bb_min != bounds.bb_min)
        {
            return false;
        }
        if (cache.resolution != resolution)
        {
            return false;
        }
        return true;
    };

    using json = nlohmann::json;
    SDF* sdf = new SDF();

    std::ifstream is(path, std::ios::in);
    if (!is.is_open())
    {
        printf("Failed to open file %s\n", path);
        return nullptr;
    }
    std::string json_str;
    json_str.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
    if (json_str.empty())
    {
        return nullptr;
    }
    //load json
    json j = json::parse(json_str);
    SDFCache cache;
    cache = j["sdf_cache"].get<SDFCache>();
    if (!check_validate(cache, bounds, resolution))
    {
        return nullptr;
    }

    sdf->bounds = cache.bounds;
    sdf->resolution = cache.resolution;
    sdf->local_to_uvw_mul = cache.local_to_uvw_mul;
    sdf->local_to_uvw_add = cache.local_to_uvw_add;

    uint32_t voxel_data_size = sdf->resolution * sdf->resolution * sdf->resolution * sizeof(float);
    std::vector<float> voxel_data;
    voxel_data.swap(cache.voxel_data);

    EzTextureDesc texture_desc{};
    texture_desc.width = sdf->resolution;
    texture_desc.height = sdf->resolution;
    texture_desc.depth = sdf->resolution;
    texture_desc.format = VK_FORMAT_R32_SFLOAT;
    texture_desc.image_type = VK_IMAGE_TYPE_3D;
    texture_desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    ez_create_texture(texture_desc, sdf->texture);
    ez_create_texture_view(sdf->texture, VK_IMAGE_VIEW_TYPE_3D, 0, 1, 0, 1);

    VkImageMemoryBarrier2 barrier = ez_image_barrier(sdf->texture, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    ez_pipeline_barrier(0, 0, nullptr, 1, &barrier);

    VkBufferImageCopy range{};
    range.imageExtent.width = sdf->resolution;
    range.imageExtent.height = sdf->resolution;
    range.imageExtent.depth = sdf->resolution;
    range.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.imageSubresource.mipLevel = 0;
    range.imageSubresource.baseArrayLayer = 0;
    range.imageSubresource.layerCount = 1;
    ez_update_image(sdf->texture, range, voxel_data.data());

    barrier = ez_image_barrier(sdf->texture, EZ_RESOURCE_STATE_SHADER_RESOURCE);
    ez_pipeline_barrier(0, 0, nullptr, 1, &barrier);
    voxel_data.clear();
    return sdf;
}

void save_sdf_cached(const char* path, SDF* sdf, const std::vector<float>& voxel_data)
{
    //use json
    nlohmann::json j;
    j["sdf_cache"] = SDFCache{.bounds = sdf->bounds, .resolution = sdf->resolution, .local_to_uvw_mul = sdf->local_to_uvw_mul, .local_to_uvw_add = sdf->local_to_uvw_add, .voxel_data = voxel_data};
    //dump to binary

    std::string json_str = j.dump();
    std::filesystem::path p(path);
    if (!std::filesystem::exists(p.parent_path()))
    {
        std::filesystem::create_directories(p.parent_path());
    }
    std::ofstream out(path);

    if (!out.is_open())
    {
        assert(0 && "Failed to open file");
    }
    out << json_str;
    out.close();
}