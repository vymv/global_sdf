#include "sdf.h"

glm::vec3 transform_point(const glm::vec3& point, const glm::mat4& mat)
{
    glm::vec4 p0 = glm::vec4(point.x, point.y, point.z, 1.0f);
    glm::vec4 p1 = mat * p0;
    return glm::vec3(p1.x, p1.y, p1.z) / p1.w;
}

BoundingBox get_bounds(const BoundingBox& bounds, const glm::mat4& world)
{
    glm::vec3 bb_min = glm::vec3(INF, INF, INF);
    glm::vec3 bb_max = glm::vec3(NEG_INF, NEG_INF, NEG_INF);
    glm::vec3 corners[8];
    bounds.get_corners(corners);
    for (int i = 0; i < 8; i++)
    {
        glm::vec3 p = transform_point(corners[i], world);
        bb_min = glm::min(bb_min, p);
        bb_max = glm::max(bb_max, p);
    }
    return BoundingBox(bb_min, bb_max);
}