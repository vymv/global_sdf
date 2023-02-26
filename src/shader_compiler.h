#pragma once

#include "ez_vulkan.h"
#include <string>
#include <vector>

void compile_shader(const std::string& file_path, void** data, uint32_t& size);

void compile_shader(const std::string& file_path, const std::vector<std::string>& macros, void** data, uint32_t& size);

void compile_shader(const std::string& file_path, EzShader& shader);

void compile_shader(const std::string& file_path, const std::vector<std::string>& macros, EzShader& shader);