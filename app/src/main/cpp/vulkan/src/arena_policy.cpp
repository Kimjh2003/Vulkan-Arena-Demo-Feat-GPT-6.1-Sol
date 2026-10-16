#include "arena_api.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace vk3d::arena {
namespace {void finite(float v){if(!std::isfinite(v))throw std::invalid_argument("Nonfinite arena data");}}
Instance instance(const Box& b) {
    for(float v:{b.x,b.y,b.z,b.w,b.h,b.d,b.r,b.g,b.b,b.yaw,b.emissive})finite(v);
    if(b.w<=0 || b.h<=0 || b.d<=0)throw std::invalid_argument("Positive box extent required");
    return {{b.x,b.y,b.z},{b.w,b.h,b.d},{b.r,b.g,b.b},
            {std::cos(b.yaw),std::sin(b.yaw)},{b.textured?1.f:0.f,b.emissive}};
}
FrameData frameData(std::uint32_t w,std::uint32_t h,float shake,float phase) {
    if(!w||!h)throw std::invalid_argument("Empty frame extent");
    finite(shake);
    finite(phase);
    constexpr float nearZ=.1f,farZ=50.f,scale=1.f/.46630766f;
    return {{scale/(float(w)/h),scale,farZ/(farZ-nearZ),-farZ*nearZ/(farZ-nearZ)},
            {std::sin(phase*2.4f)*shake,std::cos(phase*1.7f)*shake,0,0}};
}
GroundPoint screenToGround(float x,float y,float aspect) {
    finite(x);
    finite(y);
    finite(aspect);
    if(aspect<=0)throw std::invalid_argument("Positive aspect required");
    const float rx=(x*2-1)*.46630766f*aspect;
    const float ry=-.70710678f+(1-y*2)*.46630766f*.70710678f;
    const float rz=-.70710678f-(1-y*2)*.46630766f*.70710678f;
    if(ry>=-1e-6f)throw std::invalid_argument("Screen ray misses ground");
    const float distance=-7/ry;return {rx*distance,7+rz*distance}; // World coordinates, no gameplay clamping.
}
void validateMips(const std::vector<Mip>& m,TextureFamily f,std::uint32_t w,std::uint32_t h) {
    if(!w||!h||m.empty())throw std::invalid_argument("Missing texture mip data");
    for(std::size_t i=0;i<m.size();i++) {
        const auto& v=m[i];
    if(v.width!=w||v.height!=h||!v.payload.data)throw std::invalid_argument("Mip dimensions/data mismatch");
        if(f==TextureFamily::automatic)throw std::invalid_argument("Concrete mip family required");
        const std::uint64_t units=f==TextureFamily::rgba8?std::uint64_t(w)*h:((std::uint64_t(w)+3)/4)*((std::uint64_t(h)+3)/4);
        const std::uint64_t perUnit=f==TextureFamily::rgba8?4:f==TextureFamily::astc?16:8;
        if(units>std::numeric_limits<std::uint64_t>::max()/perUnit)throw std::invalid_argument("Mip size overflow");
        const std::uint64_t bytes=f==TextureFamily::rgba8?std::uint64_t(w)*h*4:
            ((std::uint64_t(w)+3)/4)*((std::uint64_t(h)+3)/4)*(f==TextureFamily::astc?16:8);
        if(bytes!=v.payload.size)throw std::invalid_argument("Mip byte count mismatch");
        if(i+1<m.size() && w==1 && h==1)throw std::invalid_argument("Too many mip levels");
        w=std::max(1u,w/2);h=std::max(1u,h/2);
    }
}
TextureDecision chooseFamily(const TextureAsset& a,bool astc,bool etc,bool rgba) {
    auto available=[&](TextureFamily f){return f==TextureFamily::astc?astc&&!a.astc4x4.empty():
        f==TextureFamily::etc2?etc&&!a.etc2rgb.empty():f==TextureFamily::rgba8?rgba&&!a.rgba8.empty():false;};
    auto format=[&](TextureFamily f){return f==TextureFamily::astc?(a.srgb?VK_FORMAT_ASTC_4x4_SRGB_BLOCK:VK_FORMAT_ASTC_4x4_UNORM_BLOCK):
        f==TextureFamily::etc2?(a.srgb?VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK):
        (a.srgb?VK_FORMAT_R8G8B8A8_SRGB:VK_FORMAT_R8G8B8A8_UNORM);};
    if(available(a.preferred))return {a.preferred,format(a.preferred)};
    for(auto f:{TextureFamily::astc,TextureFamily::etc2,TextureFamily::rgba8})if(available(f))return {f,format(f)};
    throw std::runtime_error("No supported texture payload; provide RGBA8 fallback");
}
}
