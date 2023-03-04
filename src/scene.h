#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "ez_vulkan.h"

struct Primitive
{
    VkIndexType index_type;
    EzBuffer index_buffer;
    EzBuffer vertex_buffer;
};

struct Mesh
{
    std::string name;
    std::vector<Primitive*> primitives;
};

struct Node
{
    std::string name;
    Mesh* mesh = nullptr;
};

class Scene
{
public:
    ~Scene();

public:
    std::vector<Node*> nodes;
    std::vector<Mesh*> meshes;
    std::vector<Primitive*> primitives;
};