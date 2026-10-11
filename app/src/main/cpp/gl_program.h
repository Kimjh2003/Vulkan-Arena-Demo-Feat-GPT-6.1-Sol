#pragma once
#include <GLES3/gl32.h>
#include <initializer_list>
namespace graphics {
struct ShaderSource { GLenum type; const char* source; };
// Caller owns the returned program. Temporary shaders are always released.
GLuint createProgram(std::initializer_list<ShaderSource> stages);
}
