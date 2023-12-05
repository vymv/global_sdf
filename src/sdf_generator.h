#pragma once

#include "sdf.h"
#include <math/bounding_box.h>
#include <string>
#define USE_SDF_CACHE 0
SDF* generate_sdf(const BoundingBox& bbox, uint32_t resolution, uint32_t vertex_count, float* vertices, float* normal_data, uint32_t index_count, uint8_t* indices, VkIndexType index_type, const std::string& file_path);
SDF* load_sdf_cached(const char* path, const BoundingBox& bounds, uint32_t resolution);
void save_sdf_cached(const char* path, SDF* sdf, const std::vector<float>& voxel_data);