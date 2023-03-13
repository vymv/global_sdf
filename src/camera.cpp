#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

glm::mat4 Camera::get_proj_matrix()
{
    if (_proj_dirty)
    {
        _proj_matrix = glm::perspective(_fov, _aspect, _near, _far);
        // reverse y axis
        _proj_matrix[1][1] *= -1;
        _proj_dirty = false;
    }
    return _proj_matrix;
}

glm::mat4 Camera::get_view_matrix()
{
    update_transform();
    return _view_matrix;
}

glm::mat4 Camera::get_transform()
{
    update_transform();
    return _transform;
}

void Camera::update_transform()
{
    if (_transform_dirty)
    {
        glm::mat4 r, t, s;
        r = glm::toMat4(glm::quat(_euler));
        t = glm::translate(glm::mat4(1.0), _translation);
        s = glm::scale(glm::mat4(1.0), _scale);
        _transform = t * r * s;
        _view_matrix = glm::inverse(_transform);
        _transform_dirty = false;
    }
}

void Camera::set_fov(float fov)
{
    _fov = fov;
    _proj_dirty = true;
}

void Camera::set_near_far(float near, float far)
{
    _near = near;
    _far = far;
    _proj_dirty = true;
}

void Camera::set_aspect(float aspect)
{
    _aspect = aspect;
    _proj_dirty = true;
}

void Camera::set_translation(const glm::vec3& translation)
{
    _translation = translation;
    _transform_dirty = true;
}

void Camera::set_scale(const glm::vec3& scale)
{
    _scale = scale;
    _transform_dirty = true;
}

void Camera::set_euler(const glm::vec3& euler)
{
    _euler = euler;
    _transform_dirty = true;
}