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
        glm::vec4 bounds_position[GLOBAL_SDF_MAX_OBJECT_COUNT];
        glm::vec4 bounds_distance[GLOBAL_SDF_MAX_OBJECT_COUNT];
        // glm::mat4 model_matrix_inv[GLOBAL_SDF_MAX_OBJECT_COUNT];
        glm::mat4 world_to_volume[GLOBAL_SDF_MAX_OBJECT_COUNT];
        glm::vec4 volume_to_uvw_mul[GLOBAL_SDF_MAX_OBJECT_COUNT];
        glm::vec4 volume_to_uvw_add[GLOBAL_SDF_MAX_OBJECT_COUNT];

        float resolution;
        float global_sdf_distance;
        int mesh_count;
    };

    Renderer* _renderer;
    std::vector<EzTexture> _object_textures;
    MeshSignDistanceFieldData _upload_meshsdf_datas;
    EzBuffer _upload_meshsdf_buffer = VK_NULL_HANDLE;

    EzSampler _sampler = VK_NULL_HANDLE;
    EzTexture _empty_texture = VK_NULL_HANDLE;
};