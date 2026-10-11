#pragma once
#include "compressed_texture.h"
#include "gl_capabilities.h"
namespace etc {
enum class Format { Etc1Rgb, Etc2Rgb, Etc2Srgb, Etc2RgbA1, Etc2SrgbA1,
                    Etc2Rgba, Etc2Srgba, EacR, EacSignedR, EacRg, EacSignedRg };
struct Capabilities { bool etc1=false, etc2=false; float anisotropy=1.f; };
Capabilities queryCapabilities();
Capabilities queryCapabilities(const graphics::GlCapabilities&);
const char* name(Format);
int blockBytes(Format);
GLuint upload(const Capabilities&,Format,const std::vector<graphics::CompressedMip>&);
// Procedural ETC1-compatible blocks, uploaded as ETC2 RGB when available.
GLuint createFloorTexture(const Capabilities&);
}
