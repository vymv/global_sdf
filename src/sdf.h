#pragma once

#include "ez_vulkan.h"
#include <glm/glm.hpp>
#include <math/bounding_box.h>
#include <vector>

struct SDF
{
    BoundingBox bounds;
    uint32_t resolution;
    glm::vec3 local_to_uvw_mul;
    glm::vec3 local_to_uvw_add;
    EzTexture texture;
};

struct SDFCache
{
    BoundingBox bounds;
    uint32_t resolution;
    glm::vec3 local_to_uvw_mul;
    glm::vec3 local_to_uvw_add;
    std::vector<float> voxel_data;
};