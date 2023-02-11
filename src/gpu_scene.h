#pragma once

#include "ez_vulkan.h"

namespace GPU
{
struct Mesh
{};

struct Model
{};

class Scene
{
public:
    ~Scene();
    int add_mesh(const Mesh& mesh);
    int add_model(const Model& model);
    void build();
public:
    std::vector<Mesh> meshs;
    std::vector<Model> models;
};
}