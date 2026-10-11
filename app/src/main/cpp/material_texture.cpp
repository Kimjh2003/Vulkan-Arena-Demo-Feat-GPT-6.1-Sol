#include "material_texture.h"
#include "astc_gles.h"
#include "etc_gles.h"
#include <android/log.h>
#include <algorithm>
namespace graphics {
std::vector<CompressedMip> floorMips(bool astc) {
    std::vector<CompressedMip> mips;
    const int blockBytes=astc?16:8;
    for(int extent=64;;extent=std::max(1,extent/2)) {
        int blocks=(extent+3)/4;
        CompressedMip mip{extent,extent,std::vector<uint8_t>(blocks*blocks*blockBytes,0)};
        for(int y=0;y<blocks;++y) for(int x=0;x<blocks;++x) {
            const uint8_t base=extent<8?0xcc:((x+y)%2?0xbb:0xdd);
            auto* data=mip.bytes.data()+(y*blocks+x)*blockBytes;
            if(astc) {
                // LDR void-extent constant-color block with all-ones extent.
                data[0]=0xfc;data[1]=0xfd;
                std::fill(data+2,data+8,0xff);
                const uint8_t value=(base&15)*17+2;
                for(int c=0;c<3;++c) data[8+c*2]=data[9+c*2]=value;
                data[14]=data[15]=0xff;
            } else data[0]=data[1]=data[2]=base;
        }
        mips.push_back(std::move(mip));if(extent==1) break;
    }
    return mips;
}
MaterialTexture loadFloor(const GlCapabilities& device,TextureFamily preferred) {
    const auto etcCaps=etc::queryCapabilities(device);
    const auto astcCaps=astc::queryCapabilities(device);
    std::vector<TextureFamily> choices;
    if(preferred!=TextureFamily::Auto) choices.push_back(preferred);
    for(auto f:{TextureFamily::Astc,TextureFamily::Etc2,TextureFamily::Etc1})
        if(std::find(choices.begin(),choices.end(),f)==choices.end()) choices.push_back(f);
    for(auto f:choices) {
        if(f==TextureFamily::Etc1 && !etcCaps.etc1) continue;
        if(f==TextureFamily::Etc2 && !etcCaps.etc2) continue;
        auto block=astc::chooseFormat(astcCaps,astc::Semantic::Data,{{4,4}});
        if(f==TextureFamily::Astc && !block) continue;
        auto mips=floorMips(f==TextureFamily::Astc);
        GLuint id=f==TextureFamily::Astc?astc::upload(astcCaps,*block,astc::Semantic::Data,mips):
            etc::upload(etcCaps,f==TextureFamily::Etc1?etc::Format::Etc1Rgb:etc::Format::Etc2Rgb,mips);
        if(!id) continue;
        size_t bytes=0;for(const auto& mip:mips) bytes+=mip.bytes.size();
        __android_log_print(ANDROID_LOG_INFO,"Arena","Material texture: preferred=%d actual=%s, %zu bytes, shared sampling %.0fx",
                           static_cast<int>(preferred),f==TextureFamily::Astc?"ASTC 4x4":f==TextureFamily::Etc2?"ETC2":"ETC1",bytes,device.anisotropy);
        return {id,f,bytes};
    }
    GLuint id=0;GLint previous=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&previous);
    const uint8_t white[]={255,255,255,255};glGenTextures(1,&id);glBindTexture(GL_TEXTURE_2D,id);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D,previous);
    __android_log_print(ANDROID_LOG_INFO,"Arena","Material texture: white fallback");
    return {id,TextureFamily::Auto,4};
}
}
