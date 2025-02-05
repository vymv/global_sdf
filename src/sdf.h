#pragma once

#include "ez_vulkan.h"
#include <glm/glm.hpp>
#include <math/bounding_box.h>
#include <vector>

#define GLOBAL_SDF_RESOLUTION 768
#define GLOBAL_SDF_BRICK_SIZE 32
#define GLOBAL_SDF_MAX_OBJECT_COUNT 64
#define MESH_SDF_RESOLUTION 64
struct SDF
{
    BoundingBox bounds;
    uint32_t resolution;
    glm::vec3 local_to_uvw_mul;
    glm::vec3 local_to_uvw_add;
    EzTexture texture;
};
glm::vec3 transform_point(const glm::vec3& point, const glm::mat4& mat);
BoundingBox get_bounds(const BoundingBox& bounds, const glm::mat4& world);

struct SDFCache
{
    BoundingBox bounds;
    uint32_t resolution;
    glm::vec3 local_to_uvw_mul;
    glm::vec3 local_to_uvw_add;
    std::vector<float> voxel_data;
};
