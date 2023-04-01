#include "sdf_generator.h"

glm::vec3 closest_point_on_triangle(const glm::vec3& p, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    const glm::vec3 ab = v1 - v0;
    const glm::vec3 ac = v2 - v0;
    const glm::vec3 ap = p - v0;

    const float d1 = glm::dot(ab, ap);
    const float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
    {
        return v0;
    }

    const glm::vec3 bp = p - v1;
    const float d3 = glm::dot(ab, bp);
    const float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3)
    {
        return v1;
    }

    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        const float v = d1 / (d1 - d3);
        return v0 + v * ab;
    }

    const glm::vec3 cp = p - v2;
    const float d5 = glm::dot(ab, cp);
    const float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6)
    {
        return v2;
    }

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        const float w = d2 / (d2 - d6);
        return v0 + w * ac;
    }

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && d4 - d3 >= 0.0f && d5 - d6 >= 0.0f)
    {
        const float w = (d4 - d3) / (d4 - d3 + (d5 - d6));
        return v1 + w * (v2 - v1);
    }

    const float denom = 1.0f / (va + vb + vc);
    const float v_2 = vb * denom;
    const float w_2 = vc * denom;
    return v0 + ab * v_2 + ac * w_2;
}

bool ray_intersect_triangle(glm::vec3 ro, glm::vec3 rd, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, float& distance)
{
    const float EPSILON = 0.00001f;
    glm::vec3 e0 = v1 - v0;
    glm::vec3 e1 = v0 - v2;
    glm::vec3 n = glm::cross(e1, e0);

    float n_dot_dir = glm::dot(n, rd);
    if (abs(n_dot_dir) < EPSILON)
        return false;

    glm::vec3 e2 = (v0 - ro) / n_dot_dir;
    distance = glm::dot(n, e2);

    return distance > 0.0f;
}

SDF* generate_sdf(uint32_t resolution, glm::vec3 bb_max, glm::vec3 bb_min, uint32_t vertex_count, float* vertices, uint32_t index_count, uint32_t* indices)
{
    SDF* sdf = new SDF();

    for (int x = 0; x < resolution; ++x)
    {
        for (int y = 0; y < resolution; ++y)
        {
            for (int z = 0; z < resolution; ++z)
            {

            }
        }
    }

    return sdf;
}