#pragma once
#include <GLES3/gl32.h>
#include <array>
#include <string>
#include <vector>
namespace graphics {
// Context-local snapshot. Do not retain across EGL context recreation.
struct GlCapabilities {
    int major=3,minor=0;
    float anisotropy=1;
    std::vector<std::string> extensions;
    std::vector<GLint> compressedFormats;
    std::array<GLint,3> workgroupSize{};
    GLint workgroupInvocations=0;
    bool has(const char*) const;
};
GlCapabilities queryGlCapabilities();
}
