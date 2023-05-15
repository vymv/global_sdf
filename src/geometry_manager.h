#pragma once

#include "module.h"
#include "ez_vulkan.h"

class GeometryManager : public Module<GeometryManager>
{
public:
    void setup();

    void cleanup();

    EzBuffer get_quad_buffer() { return _quad_buffer; }

    EzBuffer get_cube_buffer() { return _cube_buffer; }

private:
    void create_quad_buffer();

    void create_cube_buffer();

    EzBuffer _quad_buffer = VK_NULL_HANDLE;
    EzBuffer _cube_buffer = VK_NULL_HANDLE;
};