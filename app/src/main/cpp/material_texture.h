#pragma once
#include "compressed_texture.h"
#include "gl_capabilities.h"
#include <cstddef>
namespace graphics {
enum class TextureFamily { Auto=0, Etc1=1, Etc2=2, Astc=3 };
struct MaterialTexture { GLuint id=0; TextureFamily family=TextureFamily::Auto; size_t bytes=0; };
// Equivalent linear grayscale pattern in each codec; no general image encoder.
std::vector<CompressedMip> floorMips(bool astc);
// Unsupported preferred codec falls back to ASTC -> ETC2 -> ETC1 -> white.
MaterialTexture loadFloor(const GlCapabilities&,TextureFamily preferred=TextureFamily::Auto);
}
