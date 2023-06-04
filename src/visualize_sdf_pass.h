#pragma once

#include "ez_vulkan.h"

class Renderer;

class VisualizeSignDistanceFieldPass
{
public:
    VisualizeSignDistanceFieldPass(Renderer* renderer);

    ~VisualizeSignDistanceFieldPass();

    void render();

private:
    Renderer* _renderer;
};