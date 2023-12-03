#pragma once

#include "event.h"
#include "module.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


enum ShowType
{
    DEFAULT,
    MESH_SDF,
    GLOBAL_SDF
};
class Input : public Module<Input>
{
public:
    void reset();

    float get_mouse_scroll_wheel() const { return _mouse_scroll_wheel; }

    bool get_mouse_button_held(uint8_t i) { return _mouse_button_held[i]; }

    bool get_mouse_button_down(uint8_t i) { return _mouse_button_down[i]; }

    bool get_mouse_button_up(uint8_t i) { return _mouse_button_up[i]; }

    ShowType get_show_type() { return show_type; }

    glm::vec2 get_mouse_position() { return _mouse_position; }

    void set_mouse_position(float x, float y);

    void set_mouse_button(int button, int action);

    void set_mouse_scroll(float offset);

    void set_keyboard(int button, int action);

    Event<void, float, float> on_mouse_position_event;
    Event<void, int, int> on_mouse_button_event;
    Event<void, float> on_mouse_scroll_event;
    Event<void, int, int> on_keyboard_event;
    ShowType show_type;

private:
    float _mouse_scroll_wheel;
    bool _mouse_button_held[3];
    bool _mouse_button_down[3];
    bool _mouse_button_up[3];
    glm::vec2 _mouse_position;
};