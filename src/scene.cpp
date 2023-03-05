#include "scene.h"

Scene* create_scene()
{
    return new Scene();
}

void destroy_scene(Scene* scene)
{
    for (auto& prim : scene->primitives)
    {
        ez_destroy_buffer(prim->index_buffer);
        ez_destroy_buffer(prim->vertex_buffer);
    }
    scene->primitives.clear();
    scene->meshes.clear();
    scene->nodes.clear();
    delete scene;
}