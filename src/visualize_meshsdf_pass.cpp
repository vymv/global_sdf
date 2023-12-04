#include "geometry_manager.h"
#include "global_sdf_pass.h"
#include "renderer.h"
#include "scene.h"
#include "shader_manager.h"
#include "visualize_sdf_pass.h"

VisualizeMeshSignDistanceFieldPass::VisualizeMeshSignDistanceFieldPass(Renderer* renderer)
{
    _renderer = renderer;
    EzSamplerDesc sampler_desc{};
    sampler_desc.address_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_desc.address_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_desc.address_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ez_create_sampler(sampler_desc, _sampler);
}

VisualizeMeshSignDistanceFieldPass::~VisualizeMeshSignDistanceFieldPass()
{
}

void VisualizeMeshSignDistanceFieldPass::render()
{
    GlobalSignDistanceFieldPass* global_sdf_pass = _renderer->_global_sdf_pass;

    ez_reset_pipeline_state();

    VkImageMemoryBarrier2 rt_barriers[] = {ez_image_barrier(_renderer->_color_rt, EZ_RESOURCE_STATE_RENDERTARGET)};
    ez_pipeline_barrier(0, 0, nullptr, 1, rt_barriers);

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
                _upload_meshsdf_datas.bounds_position_distance[upload_meshsdf_id] = prim->sdf->bounds
                                                                                        _upload_meshsdf_datas.resolution[upload_meshsdf_id] = prim->sdf->resolution;
                upload_meshsdf_id++;
            }
        }
    }

    for (int j = 0; j < GLOBAL_SDF_MAX_OBJECT_COUNT; ++j)
    {
        ez_bind_texture_array(0, _object_textures[j], 0, j);// mesh sdf
    }
    ez_bind_sampler(1, _sampler);

    ez_bind_texture(0, global_sdf_pass->get_global_sdf_texture(), 0);
    ez_bind_sampler(1, global_sdf_pass->get_sampler());
    ez_bind_buffer(2, global_sdf_pass->get_global_sdf_buffer(), global_sdf_pass->get_global_sdf_buffer()->size);
    ez_bind_buffer(3, _renderer->_view_buffer, _renderer->_view_buffer->size);
    glm::vec2 viewport((float)_renderer->_width, (float)_renderer->_height);
    ez_push_constants(&viewport, sizeof(glm::vec2), 0);

    ez_set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    ez_bind_vertex_buffer(GeometryManager::get()->get_quad_buffer());
    ez_draw(6, 0);

    ez_end_rendering();
}