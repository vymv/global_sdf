#pragma once

#include <glm/glm.hpp>
#include "ez_vulkan.h"

struct SDF
{
    float max_distance;
    glm::vec3 uvw_to_local_mul;
    glm::vec3 uvw_to_local_add;
    EzTexture texture;
};