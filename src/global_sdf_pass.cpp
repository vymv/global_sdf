#include "global_sdf_pass.h"
#include "camera.h"
#include "renderer.h"
#include "scene.h"
#include "shader_manager.h"
#include <math/bounding_box.h>

GlobalSignDistanceFieldPass::GlobalSignDistanceFieldPass(Renderer* renderer)
{
    _renderer = renderer;
    EzTextureDesc texture_desc{};
    texture_desc.width = GLOBAL_SDF_RESOLUTION;
    texture_desc.height = GLOBAL_SDF_RESOLUTION;
    texture_desc.depth = GLOBAL_SDF_RESOLUTION;
    texture_desc.format = VK_FORMAT_R16_SFLOAT;
    texture_desc.image_type = VK_IMAGE_TYPE_3D;
    texture_desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    ez_create_texture(texture_desc, _global_sdf_texture);
    ez_create_texture_view(_global_sdf_texture, VK_IMAGE_VIEW_TYPE_3D, 0, 1, 0, 1);

    texture_desc.width = 1;
    texture_desc.height = 1;
    texture_desc.depth = 1;
    ez_create_texture(texture_desc, _empty_texture);
    ez_create_texture_view(_empty_texture, VK_IMAGE_VIEW_TYPE_3D, 0, 1, 0, 1);

    EzSamplerDesc sampler_desc{};
    sampler_desc.address_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_desc.address_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_desc.address_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ez_create_sampler(sampler_desc, _sampler);

    EzBufferDesc buffer_desc{};
    buffer_desc.size = sizeof(GlobalSignDistanceFieldData);
    buffer_desc.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_desc.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ez_create_buffer(buffer_desc, _global_sdf_buffer);
}

GlobalSignDistanceFieldPass::~GlobalSignDistanceFieldPass()
{
    if (_global_sdf_texture)
        ez_destroy_texture(_global_sdf_texture);
    if (_empty_texture)
        ez_destroy_texture(_empty_texture);
    if (_sampler)
        ez_destroy_sampler(_sampler);
    if (_global_sdf_buffer)
        ez_destroy_buffer(_global_sdf_buffer);
    if (_upload_params_buffer)
        ez_destroy_buffer(_upload_params_buffer);
    if (_objects_buffer)
        ez_destroy_buffer(_objects_buffer);
}

bool GlobalSignDistanceFieldPass::BrickEq::operator()(const glm::ivec3& c0, const glm::ivec3& c1) const
{
    if (c0.x == c1.x && c0.y == c1.y && c0.z == c1.z)
        return true;
    return false;
}

void GlobalSignDistanceFieldPass::make_scene_dirty()
{
    _scene_dirty = true;
}

void GlobalSignDistanceFieldPass::render()
{
    bool need_upload = false;
    float camera_distance = 100.0f;//_renderer->_camera->get_far();
    glm::vec3 camera_center = _renderer->_camera->get_translation();
    if (_scene_dirty || _view_distance != camera_distance || glm::distance(_view_center, camera_center) >= camera_distance * 0.1)
    {
        _view_distance = camera_distance;
        _view_center = camera_center;
        _scene_dirty = true;
    }

    BoundingBox view_bounds(_view_center - _view_distance, _view_center + _view_distance);
    float voxel_size = (_view_distance * 2.0f) / (float)GLOBAL_SDF_RESOLUTION;
    float brick_size = voxel_size * GLOBAL_SDF_BRICK_SIZE;

    // 更新_object_datas、_object_textures
    if (_scene_dirty)
    {
        _bricks.clear();
        _object_datas.clear();
        _object_textures.clear();
        for (auto node : _renderer->_scene->nodes)
        {
            if (node->mesh)
            {
                for (auto prim : node->mesh->primitives)
                {
                    BoundingBox volume_bounds = prim->sdf->bounds;
                    volume_bounds = get_bounds(volume_bounds, node->transform);

                    BoundingBox object_bounds = volume_bounds;
                    object_bounds.bb_min = glm::clamp(volume_bounds.bb_min, view_bounds.bb_min, view_bounds.bb_max);
                    object_bounds.bb_min = object_bounds.bb_min - view_bounds.bb_min;
                    object_bounds.bb_max = glm::clamp(volume_bounds.bb_max, view_bounds.bb_min, view_bounds.bb_max);
                    object_bounds.bb_max = object_bounds.bb_max - view_bounds.bb_min;
                    glm::ivec3 brick_min(object_bounds.bb_min / brick_size);
                    glm::ivec3 brick_max(object_bounds.bb_max / brick_size);
                    glm::mat4 local_to_world, world_to_local, volume_to_world, world_to_volume;
                    local_to_world = node->transform;
                    world_to_local = glm::inverse(local_to_world);
                    glm::vec3 volume_bounds_half_size = volume_bounds.get_size() * 0.5f;
                    world_to_volume = glm::translate(glm::mat4(1.0f), -(volume_bounds.bb_min + volume_bounds_half_size));
                    volume_to_world = glm::inverse(world_to_volume);
                    glm::vec3 volume_to_uvw_mul = prim->sdf->local_to_uvw_mul;
                    glm::vec3 volume_to_uvw_add = prim->sdf->local_to_uvw_add + (volume_bounds.bb_min + volume_bounds_half_size) * prim->sdf->local_to_uvw_mul;

                    ObjectData obj_data{};
                    obj_data.world_to_volume = world_to_volume;
                    obj_data.volume_to_world = volume_to_world;
                    obj_data.volume_to_uvw_mul = volume_to_uvw_mul;
                    obj_data.volume_to_uvw_add = volume_to_uvw_add;
                    obj_data.volume_bounds_extent = volume_bounds_half_size;
                    _object_datas.push_back(obj_data);
                    _object_textures.push_back(prim->sdf->texture);

                    // 被object的boundingbox覆盖的brick被填充新object
                    // global sdf的voxelsize = brick / 32;
                    uint32_t obj_idx = _object_datas.size() - 1;
                    for (int x = brick_min.x; x <= brick_max.x; ++x)
                    {
                        for (int y = brick_min.y; y <= brick_max.y; ++y)
                        {
                            for (int z = brick_min.z; z <= brick_max.z; ++z)
                            {
                                Brick* brick = &_bricks[glm::ivec3(x, y, z)];// bricks是用来向GPU传数据的最小单位
                                if (brick->object_count >= GLOBAL_SDF_MAX_OBJECT_COUNT)
                                    continue;
                                brick->objects[brick->object_count++] = obj_idx;// 不是所有object都在这个brick里，只有在这个brick里的object才会被传到GPU
                            }
                        }
                    }
                }
            }
        }

        _scene_dirty = false;
        need_upload = true;
    }

    if (need_upload)
    {
        // 更新_upload_params_datas
        if (_bricks.size() > _upload_params_datas.size())
            _upload_params_datas.resize(_bricks.size());
        int upload_params_data_idx = 0;
        for (auto& brick : _bricks)
        {
            UploadParamsType& upload_params_data = _upload_params_datas[upload_params_data_idx];
            upload_params_data.brick_coord = brick.first;
            upload_params_data.max_distance = _view_distance * 2.0f;
            upload_params_data.coord_to_pos_mul = view_bounds.get_size() / (float)GLOBAL_SDF_RESOLUTION;
            upload_params_data.coord_to_pos_add = view_bounds.bb_min + voxel_size * 0.5f;
            upload_params_data.object_count = brick.second.object_count;
            for (int i = 0; i < brick.second.object_count; ++i)
            {
                upload_params_data.objects[i] = brick.second.objects[i];
            }
            upload_params_data_idx++;
        }

        // 创建object buffer
        if (!_objects_buffer || _objects_buffer->size < _object_datas.size() * sizeof(ObjectData))
        {
            if (_objects_buffer)
                ez_destroy_buffer(_objects_buffer);

            EzBufferDesc buffer_desc{};
            buffer_desc.size = _object_datas.size() * sizeof(ObjectData);
            buffer_desc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            buffer_desc.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            ez_create_buffer(buffer_desc, _objects_buffer);
        }

        // 创建upload params buffer
        if (!_upload_params_buffer || _upload_params_buffer->size < _upload_params_datas.size() * sizeof(UploadParamsType))
        {
            if (_upload_params_buffer)
                ez_destroy_buffer(_upload_params_buffer);

            EzBufferDesc buffer_desc{};
            buffer_desc.size = _upload_params_datas.size() * sizeof(UploadParamsType);
            buffer_desc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            buffer_desc.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            ez_create_buffer(buffer_desc, _upload_params_buffer);
        }

        // 更新各buffer
        _global_sdf_data.bounds_position_distance = glm::vec4(_view_center, _view_distance);
        _global_sdf_data.voxel_size = voxel_size;
        _global_sdf_data.resolution = GLOBAL_SDF_RESOLUTION;

        VkBufferMemoryBarrier2 buffer_barriers[3] = {
            ez_buffer_barrier(_objects_buffer, EZ_RESOURCE_STATE_COPY_DEST),
            ez_buffer_barrier(_upload_params_buffer, EZ_RESOURCE_STATE_COPY_DEST),
            ez_buffer_barrier(_global_sdf_buffer, EZ_RESOURCE_STATE_COPY_DEST),
        };
        ez_pipeline_barrier(0, 3, buffer_barriers, 0, nullptr);

        ez_update_buffer(_objects_buffer, _object_datas.size() * sizeof(ObjectData), 0, _object_datas.data());
        ez_update_buffer(_upload_params_buffer, _upload_params_datas.size() * sizeof(UploadParamsType), 0, _upload_params_datas.data());
        ez_update_buffer(_global_sdf_buffer, sizeof(GlobalSignDistanceFieldData), 0, &_global_sdf_data);

        buffer_barriers[0] = ez_buffer_barrier(_objects_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE);
        buffer_barriers[1] = ez_buffer_barrier(_upload_params_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE);
        buffer_barriers[2] = ez_buffer_barrier(_global_sdf_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE);
        ez_pipeline_barrier(0, 3, buffer_barriers, 0, nullptr);

        // Clear stage
        VkImageMemoryBarrier2 image_barriers[2];
        image_barriers[0] = ez_image_barrier(_global_sdf_texture, EZ_RESOURCE_STATE_COPY_DEST);
        ez_pipeline_barrier(0, 0, nullptr, 1, image_barriers);
        float clear_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        ez_clear_color_image(_global_sdf_texture, 0, clear_color);

        // Upload stage
        image_barriers[0] = ez_image_barrier(_global_sdf_texture, EZ_RESOURCE_STATE_UNORDERED_ACCESS);
        image_barriers[1] = ez_image_barrier(_empty_texture, EZ_RESOURCE_STATE_SHADER_RESOURCE);
        ez_pipeline_barrier(0, 0, nullptr, 2, image_barriers);

        ez_reset_pipeline_state();
        ez_set_compute_shader(ShaderManager::get()->get_shader("upload_global_sdf.comp"));

        // 对每个brick执行upload_global_sdf.comp
        for (int i = 0; i < _bricks.size(); ++i)
        {
            auto& upload_params_data = _upload_params_datas[i];
            ez_bind_texture(0, _global_sdf_texture, 0);

            // 绑定mesh sdf
            for (int j = 0; j < GLOBAL_SDF_MAX_OBJECT_COUNT; ++j)
            {
                if (j < upload_params_data.object_count)
                    ez_bind_texture_array(1, _object_textures[upload_params_data.objects[j]], 0, j);// mesh sdf
                else
                    ez_bind_texture_array(1, _empty_texture, 0, j);
            }
            ez_bind_sampler(2, _sampler);
            // primitive，每个brick一样
            ez_bind_buffer(3, _objects_buffer, _objects_buffer->size);
            // objectid，每个brick不一样
            ez_bind_buffer(4, _upload_params_buffer, sizeof(UploadParamsType), i * sizeof(UploadParamsType));
            // 相当于每个voxel执行一次cs
            glm::ivec3 dispatch_groups(GLOBAL_SDF_BRICK_SIZE / 8);
            ez_dispatch(dispatch_groups.x, dispatch_groups.y, dispatch_groups.z);
        }

        image_barriers[0] = ez_image_barrier(_global_sdf_texture, EZ_RESOURCE_STATE_SHADER_RESOURCE);
        ez_pipeline_barrier(0, 0, nullptr, 1, image_barriers);
    }
}