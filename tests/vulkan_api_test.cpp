#include "arena_api.hpp"
#include "../app/src/main/cpp/vulkan/src/arena_instances.hpp"
#include "../app/src/main/cpp/vulkan_surface_transform.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace vk3d;
using namespace vk3d::arena;
template<class F>void rejects(F action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}assert(caught);}
int main(){
    const auto valid=instance(Box{0,0,0,1,1,1,1,1,1});
    std::vector<Instance> batch={valid};detail::validateInstances(batch);
    batch[0].extent[1]=0;rejects([&]{detail::validateInstances(batch);});
    batch[0]=valid;batch[0].origin[0]=std::numeric_limits<float>::quiet_NaN();
    rejects([&]{detail::validateInstances(batch);});
    batch[0]=valid;batch[0].rotation[0]=2;rejects([&]{detail::validateInstances(batch);});
    batch={valid,instance(Box{0,-.1f,0,1,.1f,1,1,1,1})};
    const auto capacity=batch.capacity();assert(detail::partitionCasters(batch)==1&&batch.capacity()==capacity);
    const auto rotated=graphics::vk::surfaceGeometry({1280,720},VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR);
    assert(rotated.image.width==720&&rotated.image.height==1280&&rotated.clipTransform==1);
    const auto inverted=graphics::vk::surfaceGeometry({1280,720},VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR);
    assert(inverted.image.width==1280&&inverted.image.height==720&&inverted.clipTransform==2);
    assert(negotiateVersion(VK_MAKE_VERSION(1,4,9),VK_API_VERSION_1_1,VK_MAKE_VERSION(1,3,0))==VK_API_VERSION_1_1);
    assert(negotiateVersion(VK_MAKE_VERSION(1,4,9),VK_MAKE_VERSION(1,4,0),VK_API_VERSION_1_0)==VK_API_VERSION_1_0);
    rejects([]{negotiateVersion(0,VK_API_VERSION_1_0,VK_API_VERSION_1_0);});
    const auto f=frameData(1920,1080,0,0);
    assert(std::abs((.1f*f.projection[2]+f.projection[3])/.1f)<1e-6);
    assert(std::abs((50*f.projection[2]+f.projection[3])/50-1)<1e-6);
    const auto ground=screenToGround(.5f,.5f,16.f/9);assert(std::abs(ground.x)<1e-5&&std::abs(ground.z)<1e-5);
    // Project ground intersections back to the requested screen coordinates,
    // verifying the Y flip and fixed camera agree with the original GLES model.
    for(float x:{.1f,.5f,.9f})for(float y:{.1f,.5f,.9f}){
        const auto g=screenToGround(x,y,16.f/9);
        const float viewY=(-7-(g.z-7))*.70710678f,depth=(7-(g.z-7))*.70710678f;
        const float sx=(g.x*f.projection[0]/depth+1)*.5f;
        const float sy=(-viewY*f.projection[1]/depth+1)*.5f;
        assert(std::abs(sx-x)<1e-5&&std::abs(sy-y)<1e-5);
    }
    auto box=Box{1,0,2,1,2,3,.2f,.3f,.4f,1.5707963268f,true,0};auto i=instance(box);
    assert(std::abs(i.rotation[0])<1e-5&&std::abs(i.rotation[1]-1)<1e-5&&i.material[0]==1);
    box.w=0;rejects([&]{instance(box);});rejects([]{frameData(0,100,0,0);});
    std::uint8_t a0[64]{},a1[16]{},e0[32]{},rgba[100]{};
    TextureAsset asset;asset.width=asset.height=5;asset.astc4x4={{5,5,{a0,64}},{2,2,{a1,16}}};asset.etc2rgb={{5,5,{e0,32}}};asset.rgba8={{5,5,{rgba,100}}};
    validateMips(asset.astc4x4,TextureFamily::astc,5,5);validateMips(asset.etc2rgb,TextureFamily::etc2,5,5);
    assert(chooseFamily(asset,true,true,true).family==TextureFamily::astc);
    assert(chooseFamily(asset,false,true,true).family==TextureFamily::etc2);
    assert(chooseFamily(asset,false,false,true).family==TextureFamily::rgba8);
    asset.preferred=TextureFamily::etc2;assert(chooseFamily(asset,true,true,true).family==TextureFamily::etc2);
    asset.srgb=true;assert(chooseFamily(asset,false,false,true).format==VK_FORMAT_R8G8B8A8_SRGB);
    rejects([&]{chooseFamily(asset,false,false,false);});
    asset.astc4x4[1].width=3;rejects([&]{validateMips(asset.astc4x4,TextureFamily::astc,5,5);});
    assert(std::abs(timestampMilliseconds(250,4,8,1000)-.01)<1e-9);
    rejects([]{timestampMilliseconds(0,1,0,1);});
    std::cout<<"Arena policy, mip, projection, ABI and timestamp tests passed\n";
}
