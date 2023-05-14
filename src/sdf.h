#pragma once

#include <glm/glm.hpp>
#include <math/bounding_box.h>
#include "ez_vulkan.h"

struct SDF
{
    BoundingBox bounds;
    uint32_t resolution;
    glm::vec3 local_to_uvw_mul;
    glm::vec3 local_to_uvw_add;
    EzTexture texture;
};