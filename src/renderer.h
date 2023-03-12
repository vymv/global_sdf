#pragma once

#include "ez_vulkan.h"

struct Scene;
struct Camera;

void update_uniform_buffer(EzBuffer buffer, uint32_t size, uint32_t offset, void* data);