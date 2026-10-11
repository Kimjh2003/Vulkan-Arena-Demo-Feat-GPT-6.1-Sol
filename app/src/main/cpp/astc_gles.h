// GLES adaptation of the supplied android-vulkan ASTC policies. Apache-2.0.
#pragma once
#include <GLES3/gl32.h>
#include <array>
#include <cstdint>
#include <optional>
#include <vector>
#include "compressed_texture.h"
#include "gl_capabilities.h"

namespace astc {
enum class Semantic { Color, Normal, Mask, Data };
struct Block { int width, height; GLenum linear, srgb; };
struct Capabilities {
    int major = 3, minor = 0;
    bool compute = false;
    bool decodeUnorm8 = false;
    bool decodeRgb9e5 = false;
    std::vector<GLint> compressedFormats;
    std::array<GLint, 3> workgroupSize{};
    GLint workgroupInvocations = 0;
    float anisotropy = 1;
};
using Mip = graphics::CompressedMip;
Capabilities queryCapabilities(); // A current GLES 3.0+ context is required.
Capabilities queryCapabilities(const graphics::GlCapabilities&);
std::optional<Block> chooseFormat(const Capabilities&, Semantic,
                                 const std::vector<std::array<int, 2>>& preferences);
// Complete, precompressed LDR mip chain. Caller owns returned texture; 0 means invalid/failed.
GLuint upload(const Capabilities&, const Block&, Semantic, const std::vector<Mip>&,
              bool preferUnorm8 = false);
std::array<int, 2> chooseWorkgroup(const Capabilities&);
// Caller owns program; throws on compilation/link failure. Texture bindings are 0 and 1.
GLuint createCopyProgram(const Capabilities&);
// Both textures must be distinct, equally sized; output must have RGBA8 storage.
void copyTexture(GLuint program, const Capabilities&, GLuint source, GLuint output,
                 int width, int height);
} // namespace astc
