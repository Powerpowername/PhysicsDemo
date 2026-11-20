#pragma once

#include <glad/gl.h>

#include <filesystem>

namespace graphics
{
class Shader
{
public:
    Shader(const std::filesystem::path& vertexFile, const std::filesystem::path& fragmentFile);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    GLint uniformLocation(const char* name) const;

private:
    GLuint program_;
};
}
