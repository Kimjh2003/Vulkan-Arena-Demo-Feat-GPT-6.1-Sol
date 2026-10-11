#include "etc_gles.h"
#include "material_texture.h"
#include <GLES2/gl2ext.h>
#include <android/log.h>
#include <algorithm>
#include <cstring>
namespace etc {
namespace {
struct Spec { GLenum gl; int bytes; const char* label; };
constexpr Spec specs[] = {
    {GL_ETC1_RGB8_OES,8,"ETC1 RGB"},
    {GL_COMPRESSED_RGB8_ETC2,8,"ETC2 RGB"},
    {GL_COMPRESSED_SRGB8_ETC2,8,"ETC2 sRGB"},
    {GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,8,"ETC2 RGB+A1"},
    {GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,8,"ETC2 sRGB+A1"},
    {GL_COMPRESSED_RGBA8_ETC2_EAC,16,"ETC2 RGBA"},
    {GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,16,"ETC2 sRGBA"},
    {GL_COMPRESSED_R11_EAC,8,"EAC R11"},
    {GL_COMPRESSED_SIGNED_R11_EAC,8,"EAC signed R11"},
    {GL_COMPRESSED_RG11_EAC,16,"EAC RG11"},
    {GL_COMPRESSED_SIGNED_RG11_EAC,16,"EAC signed RG11"}
};
const Spec* spec(Format f) {
    auto i=static_cast<unsigned>(f); return i<sizeof(specs)/sizeof(specs[0])?&specs[i]:nullptr;
}
}
Capabilities queryCapabilities() { return queryCapabilities(graphics::queryGlCapabilities()); }
Capabilities queryCapabilities(const graphics::GlCapabilities& device) {
    return {device.has("GL_OES_compressed_ETC1_RGB8_texture"),device.major>=3,device.anisotropy};
}
const char* name(Format f) { const auto* s=spec(f); return s?s->label:"unknown"; }
int blockBytes(Format f) { const auto* s=spec(f); return s?s->bytes:0; }
GLuint upload(const Capabilities& caps,Format f,const std::vector<graphics::CompressedMip>& mips) {
    const auto* s=spec(f);
    if(!s || (f==Format::Etc1Rgb?!caps.etc1:!caps.etc2)) return 0;
    return graphics::uploadCompressed(s->gl,4,4,s->bytes,mips,caps.anisotropy);
}
GLuint createFloorTexture(const Capabilities& caps) {
    if(!caps.etc1 && !caps.etc2) return 0;
    auto mips=graphics::floorMips(false);
    size_t total=0;for(const auto& mip:mips) total+=mip.bytes.size();
    const Format format=caps.etc2?Format::Etc2Rgb:Format::Etc1Rgb;
    GLuint texture=upload(caps,format,mips);
    __android_log_print(ANDROID_LOG_INFO,"Arena","Floor texture: %s, 64x64, 7 mips, %zu compressed bytes, upload=%s",
                       name(format),total,texture?"ok":"failed (solid fallback)");
    return texture;
}
}
