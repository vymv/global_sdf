#pragma once

#include "module.h"
#include "ez_vulkan.h"
#include <string>
#include <vector>
#include <unordered_map>

class ShaderManager : public Module<ShaderManager>
{
public:
    void setup(const std::string& dir);

    void cleanup();

    EzShader get_shader(const std::string& file);

    EzShader get_shader(const std::string& file, const std::vector<std::string>& macros);

private:
    EzShader get_shader_internal(const std::string& file, const std::vector<std::string>& macros);

    std::string directory;
    std::unordered_map<std::size_t, EzShader> shader_dict;
};