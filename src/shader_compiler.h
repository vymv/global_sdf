#pragma once

#include "module.h"
#include <string>
#include <vector>

class ShaderCompiler : public Module<ShaderCompiler>
{
public:
    void compile(const std::string& file_path, void** data, uint32_t& size);

    void compile(const std::string& file_path, const std::vector<std::string>& macros, void** data, uint32_t& size);

private:
    void compile_internal(const std::string& file_path, const std::vector<std::string>& macros, void** data, uint32_t& size);
};