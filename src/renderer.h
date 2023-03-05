#pragma once

#include "ez_vulkan.h"

class Scene;
class Camera;

void upload_uniform_buffer(EzBuffer buffer, uint32_t size, uint32_t offset, void* data);