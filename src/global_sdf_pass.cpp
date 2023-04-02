#include "global_sdf_pass.h"
#include "scene.h"
#include "renderer.h"
#include "shader_manager.h"

GlobalSignDistanceFieldPass::GlobalSignDistanceFieldPass(Renderer* renderer)
{
    _renderer = renderer;
}

void GlobalSignDistanceFieldPass::render()
{
}