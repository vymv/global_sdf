#include "input.h"

void Input::reset()
{
    memset(_mouse_button_up, 0, sizeof(_mouse_button_up));
    memset(_mouse_button_down, 0, sizeof(_mouse_button_down));
    _mouse_scroll_wheel = 0.0;
}

void Input::set_mouse_position(float x, float y)
{
    _mouse_position = glm::vec2(x, y);
    on_mouse_position_event.dispatch(x, y);
}

void Input::set_mouse_button(int button, int action)
{
    switch (action)
    {
        case 0:
            _mouse_button_up[button] = true;
            _mouse_button_held[button] = false;
            break;
        case 1:
            _mouse_button_down[button] = true;
            _mouse_button_held[button] = true;
            break;
        default:
            break;
    }
    on_mouse_button_event.dispatch(button, action);
}

void Input::set_mouse_scroll(float offset)
{
    _mouse_scroll_wheel = offset;
    on_mouse_scroll_event.dispatch(offset);
}