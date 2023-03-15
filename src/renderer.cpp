#include "renderer.h"
#include "camera.h"
#include "scene.h"

void update_uniform_buffer(EzBuffer buffer, uint32_t size, uint32_t offset, void* data)
{
    VkBufferMemoryBarrier2 barrier = ez_buffer_barrier(buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);

    ez_update_buffer(buffer, size, offset, data);

    VkPipelineStageFlags2 stage_flags = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    VkAccessFlags2 access_flags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    barrier = ez_buffer_barrier(buffer, stage_flags, access_flags);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);
}

Renderer::~Renderer()
{
    if (_scene_buffer)
        ez_destroy_buffer(_scene_buffer);
    if (_view_buffer)
        ez_destroy_buffer(_view_buffer);
    if (_color_rt)
        ez_destroy_texture(_color_rt);
    if(_depth_rt)
        ez_destroy_texture(_depth_rt);
}

void Renderer::set_scene(Scene* scene)
{
    if (_scene != scene)
    {
        _scene = scene;
        _scene_dirty = true;
    }
}

void Renderer::set_camera(Camera* camera)
{
    _camera = camera;
}

void Renderer::update_rendertarget()
{

}

void Renderer::update_scene_buffer()
{

}

void Renderer::update_view_buffer()
{

}

void Renderer::render(EzSwapchain swapchain)
{
    if (!_scene || !_camera)
        return;

    if (swapchain->width == 0 || swapchain->height ==0)
        return;

    if (_width != swapchain->width || _height != swapchain->height)
    {
        _width = swapchain->width;
        _height = swapchain->height;
        update_rendertarget();
    }

    if (_scene_dirty)
    {
        update_scene_buffer();
        _scene_dirty = false;
    }

    update_view_buffer();


}
