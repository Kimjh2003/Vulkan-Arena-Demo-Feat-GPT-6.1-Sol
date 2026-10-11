#include "gl_program.h"
#include <vector>
#include <stdexcept>
#include <string>
namespace graphics {
GLuint createProgram(std::initializer_list<ShaderSource> stages) {
    std::vector<GLuint> shaders;
    shaders.reserve(stages.size());
    GLuint program = 0;
    try {
        for (const auto& stage : stages) {
            GLuint shader = glCreateShader(stage.type);
            if (!shader) throw std::runtime_error("Cannot allocate shader");
            shaders.push_back(shader);
            glShaderSource(shader, 1, &stage.source, nullptr);
            glCompileShader(shader);
            GLint ok = 0; glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[2048]{}; glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
                throw std::runtime_error(std::string("Shader compile error: ") + log);
            }
        }
        program = glCreateProgram();
        if (!program) throw std::runtime_error("Cannot allocate program");
        for (auto shader : shaders) glAttachShader(program, shader);
        glLinkProgram(program);
        GLint ok = 0; glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[2048]{}; glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            throw std::runtime_error(std::string("Shader link error: ") + log);
        }
        for (auto shader : shaders) { glDetachShader(program, shader); glDeleteShader(shader); }
        return program;
    } catch (...) {
        if (program) glDeleteProgram(program);
        for (auto shader : shaders) glDeleteShader(shader);
        throw;
    }
}
}
