#pragma once

#include "ez_vulkan.h"

class Renderer;

class GlobalSignDistanceFieldPass
{
public:
    GlobalSignDistanceFieldPass(Renderer* renderer);

    void render();

private:
    Renderer* _renderer;
};