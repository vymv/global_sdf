#include "shader_compiler.h"
#include "filesystem.h"

void compile_shader_internal(const std::string& file_path, const std::vector<std::string>& macros, void** data, uint32_t& size)
{
    std::string file_name = fs_filename(file_path);
    std::string parent_path = fs_parent_path(file_path);
    std::string log_file_path = parent_path + "/" + file_name + "_compile.log";
    std::string out_file_path = file_path + ".spv";

    std::string glslang_validator = getenv("VULKAN_SDK");
    glslang_validator += "/Bin/glslangValidator";

    std::string command_line;
    command_line += glslang_validator;
    char buff[256];
    sprintf(buff, R"( -V "%s" -o "%s")", file_path.c_str(), out_file_path.c_str());
    command_line += buff;
    for (const auto & macro : macros)
    {
        command_line += " \"-D" + macro + "\"";
    }
    sprintf(buff, " >\"%s\"", log_file_path.c_str());
    command_line += buff;

    if (std::system(command_line.c_str()) == 0)
    {
        fs_load_file(out_file_path, data, size);
    }
}

void compile_shader(const std::string& file_path, void** data, uint32_t& size)
{
    std::vector<std::string> macros;
    compile_shader_internal(file_path, macros, data, size);
}

void compile_shader(const std::string& file_path, const std::vector<std::string>& macros, void** data, uint32_t& size)
{
    compile_shader_internal(file_path, macros, data, size);
}

void compile_shader(const std::string& file_path, EzShader& shader)
{
    void* data;
    uint32_t size;
    std::vector<std::string> macros;
    compile_shader_internal(file_path, macros, &data, size);
    ez_create_shader(data, size, shader);

    if (data)
        free(data);
}

void compile_shader(const std::string& file_path, const std::vector<std::string>& macros, EzShader& shader)
{
    void* data;
    uint32_t size;
    compile_shader_internal(file_path, macros, &data, size);
    ez_create_shader(data, size, shader);

    if (data)
        free(data);
}