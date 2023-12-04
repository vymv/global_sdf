#pragma once

#include "ez_vulkan.h"
#include "sdf.h"

class Renderer;

class VisualizeMeshSignDistanceFieldPass
{
public:
    VisualizeMeshSignDistanceFieldPass(Renderer* renderer);

    ~VisualizeMeshSignDistanceFieldPass();

    void render();

private:
    struct MeshSignDistanceFieldData
    {
        glm::vec3 bounds_position[GLOBAL_SDF_MAX_OBJECT_COUNT];
        glm::vec3 bounds_distance[GLOBAL_SDF_MAX_OBJECT_COUNT];
        float resolution;
        float global_sdf_distance;
    };

    Renderer* _renderer;
    std::vector<EzTexture> _object_textures;
    MeshSignDistanceFieldData _upload_meshsdf_datas;
    EzBuffer _upload_meshsdf_buffer = VK_NULL_HANDLE;

    EzSampler _sampler = VK_NULL_HANDLE;
};