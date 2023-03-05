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
    glm::mat4 transform;
    Mesh* mesh = nullptr;
};

struct Scene
{
    std::vector<Node*> nodes;
    std::vector<Mesh*> meshes;
    std::vector<Primitive*> primitives;
};

Scene* create_scene();

void destroy_scene(Scene* scene);