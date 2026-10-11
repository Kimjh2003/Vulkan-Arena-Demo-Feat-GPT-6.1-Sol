#include "gl_program.h"
// GLES adaptation of the supplied android-vulkan ASTC policies. Apache-2.0.
#include "astc_gles.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

namespace astc {
namespace {
constexpr Block blocks[] = {
    {4,4,0x93B0,0x93D0},{5,4,0x93B1,0x93D1},{5,5,0x93B2,0x93D2},
    {6,5,0x93B3,0x93D3},{6,6,0x93B4,0x93D4},{8,5,0x93B5,0x93D5},
    {8,6,0x93B6,0x93D6},{8,8,0x93B7,0x93D7},{10,5,0x93B8,0x93D8},
    {10,6,0x93B9,0x93D9},{10,8,0x93BA,0x93DA},{10,10,0x93BB,0x93DB},
    {12,10,0x93BC,0x93DC},{12,12,0x93BD,0x93DD}};
bool supports(const Capabilities& caps, GLenum format) {
    return std::find(caps.compressedFormats.begin(),caps.compressedFormats.end(),
                     static_cast<GLint>(format))!=caps.compressedFormats.end();
}
}
Capabilities queryCapabilities() { return queryCapabilities(graphics::queryGlCapabilities()); }
Capabilities queryCapabilities(const graphics::GlCapabilities& device) {
    Capabilities caps;
    caps.major=device.major;caps.minor=device.minor;
    caps.compute=caps.major>3 || caps.minor>=1;
    caps.decodeUnorm8=device.has("GL_EXT_texture_compression_astc_decode_mode");
    caps.decodeRgb9e5=device.has("GL_EXT_texture_compression_astc_decode_mode_rgb9e5");
    caps.compressedFormats=device.compressedFormats;
    caps.workgroupSize=device.workgroupSize;caps.workgroupInvocations=device.workgroupInvocations;
    caps.anisotropy=device.anisotropy;
    return caps;
}
std::optional<Block> chooseFormat(const Capabilities& caps, Semantic semantic,
                                 const std::vector<std::array<int,2>>& preferences) {
    for (const auto& pref:preferences) for (const auto& block:blocks) {
        if (block.width==pref[0] && block.height==pref[1] &&
            supports(caps,semantic==Semantic::Color?block.srgb:block.linear)) return block;
    }
    return std::nullopt;
}
GLuint upload(const Capabilities& caps,const Block& requested,Semantic semantic,
              const std::vector<Mip>& mips,bool preferUnorm8) {
    const Block* block=nullptr;
    for (const auto& candidate:blocks) if (candidate.width==requested.width &&
        candidate.height==requested.height) block=&candidate;
    if (!block || mips.empty()) return 0;
    GLenum format=semantic==Semantic::Color?block->srgb:block->linear;
    if (!supports(caps,format)) return 0;
    GLuint texture=graphics::uploadCompressed(format,block->width,block->height,16,mips,caps.anisotropy);
    if (!texture) return 0;
    if (caps.decodeUnorm8 && preferUnorm8 && semantic==Semantic::Mask) {
        GLint previous=0; glGetIntegerv(GL_TEXTURE_BINDING_2D,&previous);
        glBindTexture(GL_TEXTURE_2D,texture);
        glTexParameteri(GL_TEXTURE_2D,0x8F69,GL_RGBA8);
        GLenum error=glGetError();glBindTexture(GL_TEXTURE_2D,previous);
        if(error!=GL_NO_ERROR) {glDeleteTextures(1,&texture);return 0;}
    }
    return texture;
}
std::array<int,2> chooseWorkgroup(const Capabilities& caps) {
    for (auto size:{std::array<int,2>{16,8},{8,8},{16,16}})
        if (size[0]<=caps.workgroupSize[0] && size[1]<=caps.workgroupSize[1] &&
            caps.workgroupSize[2]>=1 && size[0]*size[1]<=caps.workgroupInvocations) return size;
    throw std::runtime_error("No supported ASTC postprocess workgroup");
}
GLuint createCopyProgram(const Capabilities& caps) {
    if (!caps.compute) throw std::runtime_error("Compute requires GLES 3.1+");
    auto size=chooseWorkgroup(caps);
    std::string source=(caps.major>3 || caps.minor>=2 ? "#version 320 es\n" : "#version 310 es\n");
    source += "precision highp float;\nprecision highp int;\n"
        "layout(local_size_x="+std::to_string(size[0])+",local_size_y="+std::to_string(size[1])+
        ") in;\nlayout(binding=0) uniform highp sampler2D sourceTexture;\n"
        "layout(binding=1,rgba8) uniform writeonly highp image2D outputTexture;\n"
        "uniform ivec2 extent;\nvoid main(){ivec2 p=ivec2(gl_GlobalInvocationID.xy);"
        "if(any(greaterThanEqual(p,extent)))return;"
        "imageStore(outputTexture,p,texelFetch(sourceTexture,p,0));}\n";
    return graphics::createProgram({{GL_COMPUTE_SHADER, source.c_str()}});
}
void copyTexture(GLuint program,const Capabilities& caps,GLuint source,GLuint output,int w,int h) {
    if (!caps.compute || !program || !source || !output || source==output || w<=0 || h<=0)
        throw std::invalid_argument("Invalid ASTC copy textures or extent");
    auto size=chooseWorkgroup(caps);
    glUseProgram(program);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,source);
    glBindImageTexture(1,output,0,GL_FALSE,0,GL_WRITE_ONLY,GL_RGBA8);
    glUniform2i(glGetUniformLocation(program,"extent"),w,h);
    glDispatchCompute((w+size[0]-1)/size[0],(h+size[1]-1)/size[1],1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|GL_TEXTURE_FETCH_BARRIER_BIT|
                    GL_FRAMEBUFFER_BARRIER_BIT);
    glBindImageTexture(1,0,0,GL_FALSE,0,GL_WRITE_ONLY,GL_RGBA8);
}
} // namespace astc
