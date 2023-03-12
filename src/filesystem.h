#pragma once

#include <string>

int fs_save_file(const std::string& path, void* data, uint32_t size);

int fs_load_file(const std::string& path, void** data, uint32_t& size);

std::string fs_join(const std::string& p1, const std::string& p2);

std::string fs_join(const std::string& p1, const std::string& p2, const std::string& p3);

std::string fs_filename(const std::string& path);

std::string fs_extension(const std::string& path);

std::string fs_parent_path(const std::string& path);