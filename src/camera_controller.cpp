#include "camera_controller.h"
#include "camera.h"
#include "input.h"

CameraController::CameraController()
{
    _on_mouse_position_handle = Input::get()->on_mouse_position_event.bind(CALLBACK_2(CameraController::on_mouse_position, this));
    _on_mouse_button_handle = Input::get()->on_mouse_button_event.bind(CALLBACK_2(CameraController::on_mouse_button, this));
    _on_mouse_scroll_handle = Input::get()->on_mouse_scroll_event.bind(CALLBACK_1(CameraController::on_mouse_scroll, this));
    _on_keyboard_handle = Input::get()->on_keyboard_event.bind(CALLBACK_2(CameraController::on_keyboard, this));
}

CameraController::~CameraController()
{
    Input::get()->on_mouse_position_event.unbind(_on_mouse_position_handle);
    Input::get()->on_mouse_button_event.unbind(_on_mouse_button_handle);
    Input::get()->on_mouse_scroll_event.unbind(_on_mouse_scroll_handle);
}

void CameraController::set_camera(Camera* camera)
{
    if (_camera == camera)
        return;
    _camera = camera;
}

void CameraController::on_mouse_position(float x, float y)
{
    if (!_grabbing)
        return;

    glm::vec2 offset = _start_point - glm::vec2(x, y);
    _start_point = glm::vec2(x, y);

    const float turn_rate = 0.001f;
    glm::vec3 euler = _camera->get_euler();
    euler.x += offset.y * turn_rate;
    euler.y += offset.x * turn_rate;
    _camera->set_euler(euler);
}

void CameraController::on_mouse_button(int button, int action)
{
    glm::vec2 mouse_pos = Input::get()->get_mouse_position();
    switch (action)
    {
        case 0:
            // Button up
            if (button == 1)
            {
                end();
            }
            break;
        case 1:
            // Button down
            if (button == 1)
            {
                begin(mouse_pos.x, mouse_pos.y);
            }
            break;
        default:
            break;
    }
}

void CameraController::on_mouse_scroll(float offset)
{
    glm::vec3 camera_pos = _camera->get_translation();
    glm::mat4 camera_transform = _camera->get_transform();
    glm::vec3 camera_front = glm::vec3(camera_transform[2][0], camera_transform[2][1], camera_transform[2][2]);

    const float speed = 0.2f;
    camera_pos -= camera_front * offset * speed;
    _camera->set_translation(camera_pos);
}

void CameraController::on_keyboard(int button, int action)
{
    const float speed = 1.0f;
    glm::vec3 camera_pos = _camera->get_translation();
    glm::mat4 camera_transform = _camera->get_transform();
    glm::vec3 camera_front = glm::vec3(camera_transform[2][0], camera_transform[2][1], camera_transform[2][2]);
    glm::vec3 camera_up = glm::vec3(camera_transform[1][0], camera_transform[1][1], camera_transform[1][2]);
    glm::vec3 camera_right = glm::vec3(camera_transform[0][0], camera_transform[0][1], camera_transform[0][2]);

    if (action == GLFW_PRESS)
    {
        _pressed_keys.insert(button);// Add the pressed key to the set
    }
    else if (action == GLFW_RELEASE)
    {
        _pressed_keys.erase(button);// Remove the released key from the set
    }

    // Update camera position based on keys being held down
    for (int key : _pressed_keys)
    {
        switch (key)
        {
            case GLFW_KEY_W:
                camera_pos -= camera_front * speed;
                break;
            case GLFW_KEY_S:
                camera_pos += camera_front * speed;
                break;
            case GLFW_KEY_A:
                camera_pos -= camera_right * speed;
                break;
            case GLFW_KEY_D:
                camera_pos += camera_right * speed;
                break;
            case GLFW_KEY_Q:
                camera_pos -= camera_up * speed;
                break;
            case GLFW_KEY_E:
                camera_pos += camera_up * speed;
                break;
            default:
                break;
        }
    }

    _camera->set_translation(camera_pos);
}