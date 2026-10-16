#include "vulkan_common.hpp"
#include <algorithm>
#include <string>
namespace vk3d {
VulkanError::VulkanError(VkResult v,const char* op):std::runtime_error(std::string(op)+" VkResult="+std::to_string(v)),result(v){}
std::uint32_t negotiateVersion(std::uint32_t loader,std::uint32_t instance,std::uint32_t gpu) {
    const auto v=std::min<std::uint32_t>({loader,instance,gpu,VK_MAKE_VERSION(1,4,0)});
    if(v<VK_API_VERSION_1_0) throw std::runtime_error("Vulkan 1.0 required");
    return VK_MAKE_VERSION(VK_VERSION_MAJOR(v),VK_VERSION_MINOR(v),0);
}
AstcImage parseAstc(Bytes f,bool srgb) {
    if(!f.data || f.size<16 || f.data[0]!=0x13 || f.data[1]!=0xab || f.data[2]!=0xa1 || f.data[3]!=0x5c)
        throw std::invalid_argument("Invalid ASTC header");
    constexpr unsigned blocks[][2]={{4,4},{5,4},{5,5},{6,5},{6,6},{8,5},{8,6},{8,8},{10,5},{10,6},{10,8},{10,10},{12,10},{12,12}};
    int index=-1;
    for(unsigned i=0;i<14;i++) if(f.data[4]==blocks[i][0] && f.data[5]==blocks[i][1]) index=i;
    auto le24=[&](unsigned offset){return std::uint32_t(f.data[offset]) | (std::uint32_t(f.data[offset+1])<<8) | (std::uint32_t(f.data[offset+2])<<16);};
    const auto w=le24(7),h=le24(10),d=le24(13);
    if(index<0 || f.data[6]!=1 || d!=1 || !w || !h) throw std::invalid_argument("Only 2D LDR ASTC blocks supported");
    const std::uint64_t bytes=((std::uint64_t(w)+f.data[4]-1)/f.data[4])*((std::uint64_t(h)+f.data[5]-1)/f.data[5])*16;
    if(bytes!=f.size-16) throw std::invalid_argument("ASTC block payload size mismatch");
    auto format=static_cast<VkFormat>(int(VK_FORMAT_ASTC_4x4_UNORM_BLOCK)+index*2+(srgb?1:0));
    return {w,h,format,{f.data+16,f.size-16}};
}
}
