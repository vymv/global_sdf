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

    struct MeshSignDistanceFieldData
    {
        glm::vec4 bounds_position_distance;
        float voxel_size;
        float resolution;
    };
    MeshSignDistanceFieldData _mesh_sdf_data;

private:
    struct MeshSignDistanceFieldData
    {
        glm::vec4 bounds_position_distance[GLOBAL_SDF_MAX_OBJECT_COUNT];
        float voxel_size[GLOBAL_SDF_MAX_OBJECT_COUNT];
        float resolution;
        float global_sdf_resolution;
    };

    Renderer* _renderer;
    std::vector<EzTexture> _object_textures;
    MeshSignDistanceFieldData _upload_meshsdf_datas;
    EzSampler _sampler = VK_NULL_HANDLE;
};