#include "visualize_meshsdf_pass.h"
#include "geometry_manager.h"
#include "global_sdf_pass.h"
#include "renderer.h"
#include "scene.h"
#include "shader_manager.h"

VisualizeMeshSignDistanceFieldPass::VisualizeMeshSignDistanceFieldPass(Renderer* renderer)
{
    _renderer = renderer;
    EzSamplerDesc sampler_desc{};
    sampler_desc.address_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_desc.address_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_desc.address_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ez_create_sampler(sampler_desc, _sampler);

    EzTextureDesc texture_desc{};
    texture_desc.width = 1;
    texture_desc.height = 1;
    texture_desc.depth = 1;
    texture_desc.format = VK_FORMAT_R16_SFLOAT;
    texture_desc.image_type = VK_IMAGE_TYPE_3D;
    texture_desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    ez_create_texture(texture_desc, _empty_texture);
    ez_create_texture_view(_empty_texture, VK_IMAGE_VIEW_TYPE_3D, 0, 1, 0, 1);
}

VisualizeMeshSignDistanceFieldPass::~VisualizeMeshSignDistanceFieldPass()
{
    if (_empty_texture)
        ez_destroy_texture(_empty_texture);
}

void VisualizeMeshSignDistanceFieldPass::render()
{
    GlobalSignDistanceFieldPass* global_sdf_pass = _renderer->_global_sdf_pass;

    ez_reset_pipeline_state();

    VkImageMemoryBarrier2 rt_barriers[] = {ez_image_barrier(_renderer->_color_rt, EZ_RESOURCE_STATE_RENDERTARGET)};
    ez_pipeline_barrier(0, 0, nullptr, 1, rt_barriers);

    _object_textures.clear();
    int upload_meshsdf_id = 0;
    for (auto node : _renderer->_scene->nodes)
    {
        if (node->mesh)
        {
            for (auto prim : node->mesh->primitives)
            {
                _object_textures.push_back(prim->sdf->texture);
                BoundingBox volume_bounds = prim->sdf->bounds;
                volume_bounds = get_bounds(volume_bounds, node->transform);
                _upload_meshsdf_datas.bounds_position[upload_meshsdf_id] = volume_bounds.get_center();
                _upload_meshsdf_datas.bounds_distance[upload_meshsdf_id] = volume_bounds.get_size() / 2.0f;

                upload_meshsdf_id++;
                if (upload_meshsdf_id == GLOBAL_SDF_MAX_OBJECT_COUNT)
                    break;
            }
        }
        if (upload_meshsdf_id == GLOBAL_SDF_MAX_OBJECT_COUNT)
            break;
    }
    _upload_meshsdf_datas.resolution = MESH_SDF_RESOLUTION;
    _upload_meshsdf_datas.global_sdf_distance = global_sdf_pass->get_view_distance();
    _upload_meshsdf_datas.mesh_count = upload_meshsdf_id;

    // 创建upload mesh buffer
    if (!_upload_meshsdf_buffer || _upload_meshsdf_buffer->size < sizeof(MeshSignDistanceFieldData))
    {
        if (_upload_meshsdf_buffer)
            ez_destroy_buffer(_upload_meshsdf_buffer);

        EzBufferDesc buffer_desc{};
        buffer_desc.size = sizeof(MeshSignDistanceFieldData);
        buffer_desc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        buffer_desc.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        ez_create_buffer(buffer_desc, _upload_meshsdf_buffer);
    }

    VkBufferMemoryBarrier2 buffer_barriers[1] = {
        ez_buffer_barrier(_upload_meshsdf_buffer, EZ_RESOURCE_STATE_COPY_DEST),
    };
    ez_pipeline_barrier(0, 1, buffer_barriers, 0, nullptr);

    ez_update_buffer(_upload_meshsdf_buffer, sizeof(MeshSignDistanceFieldData), 0, &_upload_meshsdf_datas);

    VkImageMemoryBarrier2 image_barriers[1] = {
        ez_image_barrier(_empty_texture, EZ_RESOURCE_STATE_SHADER_RESOURCE),
    };

    buffer_barriers[0] = ez_buffer_barrier(_upload_meshsdf_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE);
    ez_pipeline_barrier(0, 1, buffer_barriers, 1, image_barriers);

    EzRenderingAttachmentInfo color_info{};
    color_info.texture = _renderer->_color_rt;
    color_info.clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};
    EzRenderingInfo rendering_info{};
    rendering_info.width = _renderer->_width;
    rendering_info.height = _renderer->_height;
    rendering_info.colors.push_back(color_info);
    ez_begin_rendering(rendering_info);

    ez_set_viewport(0, 0, (float)_renderer->_width, (float)_renderer->_height);
    ez_set_scissor(0, 0, (int32_t)_renderer->_width, (int32_t)_renderer->_height);

    ez_set_vertex_shader(ShaderManager::get()->get_shader("visualize_sdf.vert"));
    ez_set_fragment_shader(ShaderManager::get()->get_shader("visualize_mesh_sdf.frag"));

    ez_set_vertex_binding(0, 20);
    ez_set_vertex_attrib(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
    ez_set_vertex_attrib(0, 1, VK_FORMAT_R32G32_SFLOAT, 12);

    for (int j = 0; j < GLOBAL_SDF_MAX_OBJECT_COUNT; ++j)
    {
        if (j < upload_meshsdf_id)
            ez_bind_texture_array(0, _object_textures[j], 0, j);// mesh sdf
        else
            ez_bind_texture_array(0, _empty_texture, 0, j);
    }
    ez_bind_sampler(1, _sampler);
    ez_bind_buffer(2, _upload_meshsdf_buffer, _upload_meshsdf_buffer->size);
    ez_bind_buffer(3, _renderer->_view_buffer, _renderer->_view_buffer->size);
    glm::vec2 viewport((float)_renderer->_width, (float)_renderer->_height);
    ez_push_constants(&viewport, sizeof(glm::vec2), 0);

    ez_set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    ez_bind_vertex_buffer(GeometryManager::get()->get_quad_buffer());
    ez_draw(6, 0);

    ez_end_rendering();
}