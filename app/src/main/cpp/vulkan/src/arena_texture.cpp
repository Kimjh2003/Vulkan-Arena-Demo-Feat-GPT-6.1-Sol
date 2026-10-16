#include "arena_gpu.hpp"
#include <limits>
#include <cmath>
namespace vk3d::arena {
using namespace gpu;
struct TextureStore::Impl {
    VkPhysicalDevice p;VkDevice d;bool astc,etc;float anisotropy=1;
    struct Texture {Image image;VkSampler sampler{};TextureFamily family;};
    std::vector<Texture> textures;std::vector<Buffer> staging;
    ~Impl(){for(auto& t:textures){if(t.sampler)vkDestroySampler(d,t.sampler,nullptr);destroy(d,t.image);}for(auto& b:staging)destroy(d,b);}
};
TextureStore::TextureStore(VkPhysicalDevice p,VkDevice d,bool astc,bool etc,bool anisotropy,float requested):impl(new Impl{p,d,astc,etc,1,{},{}}){
    if(!p||!d)throw std::invalid_argument("Missing texture device");
    VkPhysicalDeviceFeatures f{};vkGetPhysicalDeviceFeatures(p,&f);
    if(!std::isfinite(requested)||requested<1)throw std::invalid_argument("Invalid anisotropy request");
    if(anisotropy&&!f.samplerAnisotropy)throw std::invalid_argument("Sampler anisotropy unsupported");
    VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(p,&properties);
    impl->anisotropy=anisotropy?std::min(requested,properties.limits.maxSamplerAnisotropy):1;
    if((astc&&!f.textureCompressionASTC_LDR)||(etc&&!f.textureCompressionETC2))throw std::invalid_argument("Texture feature unsupported by physical device");
}
TextureStore::~TextureStore()=default;
std::uint32_t TextureStore::upload(VkCommandBuffer cmd,const TextureAsset& a){
    if(!cmd||!a.width||!a.height)throw std::invalid_argument("Invalid texture upload");
    auto& s=*impl;
    const auto usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    auto usable=[&](TextureFamily f,VkFormat format,const std::vector<Mip>& m,bool enabled){
        if(!enabled||m.empty())return false;
    validateMips(m,f,a.width,a.height);
        if(m.size()>UINT32_MAX)throw std::invalid_argument("Mip count overflow");
        VkFormatProperties fp{};vkGetPhysicalDeviceFormatProperties(s.p,format,&fp);
        return bool(fp.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)&&supported(s.p,format,usage,a.width,a.height,static_cast<std::uint32_t>(m.size()));
    };
    const auto af=a.srgb?VK_FORMAT_ASTC_4x4_SRGB_BLOCK:VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
    const auto ef=a.srgb?VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    const auto rf=a.srgb?VK_FORMAT_R8G8B8A8_SRGB:VK_FORMAT_R8G8B8A8_UNORM;
    const auto choice=chooseFamily(a,usable(TextureFamily::astc,af,a.astc4x4,s.astc),usable(TextureFamily::etc2,ef,a.etc2rgb,s.etc),usable(TextureFamily::rgba8,rf,a.rgba8,true));
    const auto& m=choice.family==TextureFamily::astc?a.astc4x4:choice.family==TextureFamily::etc2?a.etc2rgb:a.rgba8;
    const auto levels=static_cast<std::uint32_t>(m.size());std::vector<std::uint8_t> bytes;std::vector<VkBufferImageCopy> copies;
    for(std::uint32_t i=0;i<levels;i++){
        // 16-byte alignment satisfies all three block sizes and bufferOffset multiple-of-4.
        const auto aligned=(bytes.size()+15)&~std::size_t(15);bytes.resize(aligned);
        VkBufferImageCopy copy{};copy.bufferOffset=aligned;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,i,0,1};copy.imageExtent={m[i].width,m[i].height,1};copies.push_back(copy);
        bytes.insert(bytes.end(),m[i].payload.data,m[i].payload.data+m[i].payload.size);
    }
    Impl::Texture t{};t.family=choice.family;Buffer upload{};
    try{
        t.image=image(s.p,s.d,choice.format,{a.width,a.height},levels,usage,VK_IMAGE_ASPECT_COLOR_BIT);
        VkFormatProperties fp{};vkGetPhysicalDeviceFormatProperties(s.p,choice.format,&fp);bool linear=fp.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        VkSamplerCreateInfo si{};si.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;si.magFilter=si.minFilter=linear?VK_FILTER_LINEAR:VK_FILTER_NEAREST;si.mipmapMode=linear?VK_SAMPLER_MIPMAP_MODE_LINEAR:VK_SAMPLER_MIPMAP_MODE_NEAREST;si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;si.maxLod=float(levels-1);si.anisotropyEnable=linear&&s.anisotropy>1;si.maxAnisotropy=si.anisotropyEnable?s.anisotropy:1;
        check(vkCreateSampler(s.d,&si,nullptr,&t.sampler),"arena material sampler");
        upload=buffer(s.p,s.d,bytes.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT);write(s.d,upload,bytes.data(),bytes.size());
        s.textures.reserve(s.textures.size()+1);s.staging.reserve(s.staging.size()+1);
        VkImageMemoryBarrier b{};b.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;b.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=t.image.handle;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,levels,0,1};
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&b);
        vkCmdCopyBufferToImage(cmd,upload.handle,t.image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,levels,copies.data());
        b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&b);
        s.textures.push_back(t);s.staging.push_back(upload);
    }catch(...){if(t.sampler)vkDestroySampler(s.d,t.sampler,nullptr);destroy(s.d,t.image);destroy(s.d,upload);
    throw;}
    return static_cast<std::uint32_t>(s.textures.size()-1);
}
SampledImage TextureStore::sampled(std::uint32_t id)const{const auto& t=impl->textures.at(id);return {t.image.view,t.sampler};}
TextureFamily TextureStore::family(std::uint32_t id)const{return impl->textures.at(id).family;}
void TextureStore::releaseStaging(){for(auto& b:impl->staging)destroy(impl->d,b);impl->staging.clear();}
}
