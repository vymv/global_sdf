#include "gpu_scene.h"

namespace GPU
{
Scene::~Scene()
{
    meshs.clear();
    models.clear();
}

int Scene::add_mesh(const GPU::Mesh& mesh)
{
    meshs.push_back(mesh);
    return int(meshs.size()) - 1;
}

int Scene::add_model(const GPU::Model& model)
{
    models.push_back(model);
    return int(models.size()) - 1;
}

void Scene::build()
{}
}