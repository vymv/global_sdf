#pragma once

#include "ez_vulkan.h"
#include "sdf.h"
#include <core/hash.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

class Renderer;

class GlobalSignDistanceFieldPass
{
public:
    explicit GlobalSignDistanceFieldPass(Renderer* renderer);

    ~GlobalSignDistanceFieldPass();

    void make_scene_dirty();

    void render();

    EzTexture get_global_sdf_texture() { return _global_sdf_texture; }

    EzBuffer get_global_sdf_buffer() { return _global_sdf_buffer; }

    float get_view_distance() { return _view_distance * 2.0; }

    EzSampler get_sampler() { return _sampler; }

private:
    Renderer* _renderer;
    bool _scene_dirty{};
    float _view_distance{};
    glm::vec3 _view_center{};
    EzTexture _global_sdf_texture = VK_NULL_HANDLE;

    struct GlobalSignDistanceFieldData
    {
        glm::vec4 bounds_position_distance;
        float voxel_size;
        float resolution;
    };
    GlobalSignDistanceFieldData _global_sdf_data;
    EzBuffer _global_sdf_buffer;

    struct ObjectData
    {
        glm::mat4 world_to_volume;
        glm::mat4 volume_to_world;
        glm::vec3 volume_to_uvw_mul;
        float pad0;
        glm::vec3 volume_to_uvw_add;
        float pad1;
        glm::vec3 volume_bounds_extent;
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