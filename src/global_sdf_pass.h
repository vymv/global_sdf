#pragma once

#include "ez_vulkan.h"
#include <core/hash.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>

#define GLOBAL_SDF_RESOLUTION 128
#define GLOBAL_SDF_BRICK_SIZE 32
#define GLOBAL_SDF_MAX_OBJECT_COUNT 16

class Renderer;

class GlobalSignDistanceFieldPass
{
public:
    GlobalSignDistanceFieldPass(Renderer* renderer);

    ~GlobalSignDistanceFieldPass();

    void make_scene_dirty();

    void render();

private:
    Renderer* _renderer;
    EzTexture _global_sdf_texture = VK_NULL_HANDLE;
    bool _scene_dirty;
    float _view_distance;
    glm::vec3 _view_center;

    struct ObjectData
    {
        glm::mat4 world_to_volume;
        glm::mat4 volume_to_world;
        glm::vec3 volume_to_uvw_mul;
        float pad0;
        glm::vec3 volume_to_uvw_add;
        float pad1;
        glm::vec3 volume_bounds_size;
        float pad2;
    };
    std::vector<ObjectData> _object_datas;
    EzBuffer _objects_buffer = VK_NULL_HANDLE;
    std::vector<EzTexture> _object_textures;
    EzTexture _empty_texture = VK_NULL_HANDLE;
    EzSampler _sampler = VK_NULL_HANDLE;

    struct UploadParamsType
    {
        glm::ivec3 brick_coord;
        float max_distance;
        glm::vec3 coord_to_pos_mul;
        float pad0;
        glm::vec3 coord_to_pos_add;
        uint32_t object_count;
        uint32_t objects[GLOBAL_SDF_MAX_OBJECT_COUNT];
    };
    std::vector<UploadParamsType> _upload_params_datas;
    EzBuffer _upload_params_buffer = VK_NULL_HANDLE;

    struct Brick
    {
        uint32_t object_count;
        uint32_t objects[GLOBAL_SDF_MAX_OBJECT_COUNT];
    };
    struct BrickEq
    {
        bool operator()(const glm::ivec3& c0, const glm::ivec3& c1) const;
    };
    std::unordered_map<glm::ivec3, Brick, MurmurHash<glm::ivec3>, BrickEq> _bricks;
};