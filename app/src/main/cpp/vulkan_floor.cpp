#include "vulkan_floor.h"
#include <algorithm>
namespace graphics::vk {
FloorAsset::FloorAsset(int preference){
    using namespace vk3d::arena;
    asset.width=asset.height=64;
    asset.preferred=preference==3?TextureFamily::astc:(preference==1||preference==2)?TextureFamily::etc2:TextureFamily::automatic;
    astc.reserve(7);etc.reserve(7);rgba.reserve(7);
    for(uint32_t size=64;;size=std::max(1u,size/2)){
        const uint32_t blocks=(size+3)/4;
        astc.emplace_back(blocks*blocks*16,0);etc.emplace_back(blocks*blocks*8,0);rgba.emplace_back(size*size*4,255);
        for(uint32_t y=0;y<blocks;++y)for(uint32_t x=0;x<blocks;++x){
            const uint8_t base=size<8?0xcc:((x+y)%2?0xbb:0xdd),value=(base&15)*17+2;
            auto* a=astc.back().data()+(y*blocks+x)*16;a[0]=0xfc;a[1]=0xfd;std::fill(a+2,a+8,255);
            for(int c=0;c<3;++c)a[8+c*2]=a[9+c*2]=value;a[14]=a[15]=255;
            auto* e=etc.back().data()+(y*blocks+x)*8;e[0]=e[1]=e[2]=base;
            for(uint32_t yy=y*4;yy<std::min(size,y*4+4);++yy)for(uint32_t xx=x*4;xx<std::min(size,x*4+4);++xx){auto* p=rgba.back().data()+(yy*size+xx)*4;p[0]=p[1]=p[2]=value;}
        }
        asset.astc4x4.push_back({size,size,{astc.back().data(),astc.back().size()}});
        asset.etc2rgb.push_back({size,size,{etc.back().data(),etc.back().size()}});
        asset.rgba8.push_back({size,size,{rgba.back().data(),rgba.back().size()}});
        if(size==1)break;
    }
}
}
