#include "input.h"

void Input::reset()
{
    memset(mouse_button_up, 0, sizeof(mouse_button_up));
    memset(mouse_button_down, 0, sizeof(mouse_button_down));
    mouse_scroll_wheel = 0.0;
}

void Input::set_mouse_position(float x, float y)
{
    mouse_position = glm::vec2(x, y);
    on_mouse_position_event.Dispatch(x, y);
}

void Input::set_mouse_button(int button, int action)
{
    switch (action) {
        case 0:
            mouse_button_up[button] = true;
            mouse_button_held[button] = false;
            break;
        case 1:
            mouse_button_down[button] = true;
            mouse_button_held[button] = true;
            break;
        default:
            break;
    }
    on_mouse_button_event.Dispatch(button, action);
}

void Input::set_mouse_scroll(float offset)
{
    mouse_scroll_wheel = offset;
    on_mouse_scroll_event.Dispatch(offset);
}