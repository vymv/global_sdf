#include "base_pass.h"
#include "scene.h"
#include "renderer.h"
#include "shader_manager.h"

BasePass::BasePass(Renderer* renderer)
{
    _renderer = renderer;
}

void BasePass::render()
{
    ez_reset_pipeline_state();

    VkImageMemoryBarrier2 rt_barriers[] = {
        ez_image_barrier(_renderer->_color_rt, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT),
        ez_image_barrier(_renderer->_depth_rt, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT),
    };
    ez_pipeline_barrier(0, 0, nullptr, 2, rt_barriers);

    EzRenderingAttachmentInfo color_info{};
    color_info.texture = _renderer->_color_rt;
    color_info.clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};
    EzRenderingAttachmentInfo depth_info{};
    depth_info.texture = _renderer->_depth_rt;
    depth_info.clear_value.depthStencil = {1.0f, 1};
    EzRenderingInfo rendering_info{};
    rendering_info.width = _renderer->_width;
    rendering_info.height = _renderer->_height;
    rendering_info.colors.push_back(color_info);
    rendering_info.depth.push_back(depth_info);
    ez_begin_rendering(rendering_info);

    ez_set_viewport(0, 0, (float)_renderer->_width, (float)_renderer->_height);
    ez_set_scissor(0, 0, (int32_t)_renderer->_width, (int32_t)_renderer->_height);

    ez_set_vertex_shader(ShaderManager::get()->get_shader("scene.vert"));
    ez_set_fragment_shader(ShaderManager::get()->get_shader("scene.frag"));

    ez_set_vertex_binding(0, 32);
    ez_set_vertex_attrib(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
    ez_set_vertex_attrib(0, 1, VK_FORMAT_R32G32B32_SFLOAT, 12);
    ez_set_vertex_attrib(0, 2, VK_FORMAT_R32G32_SFLOAT, 24);

    ez_bind_buffer(1, _renderer->_view_buffer, sizeof(ViewBufferType));

    for (uint32_t i = 0; i < _renderer->_scene->nodes.size(); i++)
    {
        auto node = _renderer->_scene->nodes[i];
        if (node->mesh)
        {
            ez_bind_buffer(0, _renderer->_scene_buffer, sizeof(SceneBufferType), i * sizeof(SceneBufferType));
            for (auto prim : node->mesh->primitives)
            {
                ez_set_primitive_topology(prim->topology);
                ez_bind_vertex_buffer(prim->vertex_buffer);
                ez_bind_index_buffer(prim->index_buffer, prim->index_type);
                ez_draw_indexed(prim->index_count, 0, 0);
            }
        }
    }

    ez_end_rendering();
}