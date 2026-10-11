#pragma once
#include <GLES3/gl32.h>
#include <cstdint>
#include <vector>
namespace graphics {
struct CompressedMip { int width, height; std::vector<uint8_t> bytes; };
// Complete precompressed mip chain, exact block sizes; caller owns returned texture.
// Does not encode pixels. Returns 0 for invalid payloads or upload failure.
GLuint uploadCompressed(GLenum format, int blockWidth, int blockHeight, int blockBytes,
                        const std::vector<CompressedMip>& mips, float anisotropy=1.f);
}
