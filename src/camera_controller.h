#pragma once

#include "event.h"
#include <glm/glm.hpp>
#include <set>

class Camera;

class CameraController
{
public:
    CameraController();

    ~CameraController();

    void set_camera(Camera* camera);

private:
    void begin(float x, float y)
    {
        _grabbing = true;
        _start_point = glm::vec2(x, y);
    }

    void end()
    {
        _grabbing = false;
    }

    void on_mouse_position(float x, float y);

    void on_mouse_button(int button, int action);

    void on_mouse_scroll(float offset);

    void on_keyboard(int button, int action);

    Camera* _camera = nullptr;
    bool _grabbing = false;
    glm::vec2 _start_point{};
    std::set<int> _pressed_keys;
    EventHandle _on_mouse_position_handle;
    EventHandle _on_mouse_button_handle;
    EventHandle _on_mouse_scroll_handle;
    EventHandle _on_keyboard_handle;
};