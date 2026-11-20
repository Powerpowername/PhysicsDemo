#include <graphics/Shader.hpp>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace graphics
{
namespace
{
GLuint attachStage(GLuint program, GLenum stage, const std::filesystem::path& file)
{
    const auto path = std::filesystem::path(GRAPHICS_SHADER_DIR) / file;
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("Cannot open shader: " + path.string());
    const std::string source{std::istreambuf_iterator<char>(input), {}};
    const char* text = source.c_str();

    const GLuint shader = glCreateShader(stage);
    if (!shader)
        throw std::runtime_error("Cannot create shader: " + path.string());
    glAttachShader(program, shader);
    // The program now owns the stage, including on a later compilation failure.
    glDeleteShader(shader);
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, '\0');
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        throw std::runtime_error("Cannot compile shader " + path.string() + ": " + log);
    }
    return shader;
}
}

Shader::Shader(const std::filesystem::path& vertexFile, const std::filesystem::path& fragmentFile)
    : program_(glCreateProgram())
{
    if (!program_)
        throw std::runtime_error("Cannot create OpenGL shader program");
    try
    {
        const GLuint vertex = attachStage(program_, GL_VERTEX_SHADER, vertexFile);
        const GLuint fragment = attachStage(program_, GL_FRAGMENT_SHADER, fragmentFile);
        glLinkProgram(program_);
        glDetachShader(program_, vertex);
        glDetachShader(program_, fragment);

        GLint linked = GL_FALSE;
        glGetProgramiv(program_, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            GLint length = 0;
            glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &length);
            std::string log(length, '\0');
            glGetProgramInfoLog(program_, length, nullptr, log.data());
            throw std::runtime_error("Cannot link " + vertexFile.string() + " and "
                                     + fragmentFile.string() + ": " + log);
        }
    }
    catch (...)
    {
        glDeleteProgram(program_);
        throw;
    }
}

Shader::~Shader()
{
    glDeleteProgram(program_);
}

void Shader::use() const
{
    glUseProgram(program_);
}

GLint Shader::uniformLocation(const char* name) const
{
    const GLint location = glGetUniformLocation(program_, name);
    if (location < 0)
        throw std::runtime_error(std::string("Shader has no active uniform: ") + name);
    return location;
}
}
