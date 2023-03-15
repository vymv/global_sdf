#pragma once

#include "ez_vulkan.h"
#include <glm/glm.hpp>

class Scene;
class Camera;

void update_uniform_buffer(EzBuffer buffer, uint32_t size, uint32_t offset, void* data);

struct SceneBufferDesc
{
    glm::mat4 transform;
};

struct ViewBufferDesc
{
    glm::mat4 view_matrix;
    glm::mat4 proj_matrix;
};

class Renderer
{
public:
    ~Renderer();

    void render(EzSwapchain swapchain);

    void set_scene(Scene* scene);

    void set_camera(Camera* camera);

private:
    void update_rendertarget();

    void update_scene_buffer();

    void update_view_buffer();

    uint32_t _width = 0;
    uint32_t _height = 0;
    Scene* _scene;
    bool _scene_dirty = true;
    Camera* _camera;
    EzBuffer _scene_buffer;
    EzBuffer _view_buffer;
    EzTexture _color_rt;
    EzTexture _depth_rt;
};