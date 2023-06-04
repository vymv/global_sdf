#pragma once

#include "sdf.h"
#include <math/bounding_box.h>

SDF* generate_sdf(const BoundingBox& bbox, uint32_t resolution, uint32_t vertex_count, float* vertices, uint32_t index_count, uint8_t* indices, VkIndexType index_type);