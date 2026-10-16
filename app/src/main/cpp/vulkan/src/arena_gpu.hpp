#pragma once
#include "arena_api.hpp"
#include <algorithm>
#include <cstring>
namespace vk3d::arena::gpu {
inline void check(VkResult r,const char* op){if(r!=VK_SUCCESS)throw VulkanError(r,op);}
inline std::uint32_t memoryType(VkPhysicalDevice p,std::uint32_t bits,VkMemoryPropertyFlags required,VkMemoryPropertyFlags prefer=0){
    VkPhysicalDeviceMemoryProperties m{};vkGetPhysicalDeviceMemoryProperties(p,&m);std::uint32_t fallback=UINT32_MAX;
    for(std::uint32_t i=0;i<m.memoryTypeCount;i++)if((bits&(1u<<i))&&(m.memoryTypes[i].propertyFlags&required)==required){
        if((m.memoryTypes[i].propertyFlags&prefer)==prefer)return i;
    if(fallback==UINT32_MAX)fallback=i;
    }
    if(fallback!=UINT32_MAX)return fallback;
    throw std::runtime_error("No compatible Vulkan memory type");
}
struct Buffer{VkBuffer handle{};VkDeviceMemory memory{};VkDeviceSize size{},allocationSize{},atomSize=1;void* mapped{};bool coherent=false;};
inline void destroy(VkDevice d,Buffer& b){if(b.mapped)vkUnmapMemory(d,b.memory);
    if(b.handle)vkDestroyBuffer(d,b.handle,nullptr);
    if(b.memory)vkFreeMemory(d,b.memory,nullptr);
    b={};}
inline Buffer buffer(VkPhysicalDevice p,VkDevice d,VkDeviceSize size,VkBufferUsageFlags usage){
    Buffer b{};b.size=size;try{
        VkBufferCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;ci.size=size;ci.usage=usage;ci.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateBuffer(d,&ci,nullptr,&b.handle),"arena buffer");VkMemoryRequirements req{};vkGetBufferMemoryRequirements(d,b.handle,&req);
        VkMemoryAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;ai.allocationSize=req.size;ai.memoryTypeIndex=memoryType(p,req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(d,&ai,nullptr,&b.memory),"arena buffer memory");
    check(vkBindBufferMemory(d,b.handle,b.memory,0),"arena buffer bind");
        VkPhysicalDeviceMemoryProperties properties{};vkGetPhysicalDeviceMemoryProperties(p,&properties);
        b.coherent=(properties.memoryTypes[ai.memoryTypeIndex].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)!=0;
        b.allocationSize=req.size;VkPhysicalDeviceProperties deviceProperties{};vkGetPhysicalDeviceProperties(p,&deviceProperties);
        b.atomSize=deviceProperties.limits.nonCoherentAtomSize;
        check(vkMapMemory(d,b.memory,0,VK_WHOLE_SIZE,0,&b.mapped),"arena persistent map");return b;
    }catch(...){destroy(d,b);
    throw;}
}
inline void writeAt(VkDevice d,const Buffer& b,const void* bytes,VkDeviceSize size,VkDeviceSize offset){
    if(offset>b.size||size>b.size-offset||(!bytes&&size)||!b.mapped)throw std::invalid_argument("Arena buffer overflow/unmapped");
    if(!size)return;
    std::memcpy(static_cast<unsigned char*>(b.mapped)+offset,bytes,static_cast<std::size_t>(size));
    if(!b.coherent){
        // Offset zero, changed range rounded to the device's noncoherent atom size.
        VkMappedMemoryRange range{};range.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory=b.memory;
        range.offset=offset/b.atomSize*b.atomSize;
        const auto aligned=((offset+size-1)/b.atomSize+1)*b.atomSize;
        range.size=aligned<b.allocationSize?aligned-range.offset:VK_WHOLE_SIZE;
        check(vkFlushMappedMemoryRanges(d,1,&range),"arena flush");
    }
}
inline void write(VkDevice d,const Buffer& b,const void* bytes,VkDeviceSize size){writeAt(d,b,bytes,size,0);}
inline Buffer staticVertexBuffer(VkPhysicalDevice p,VkDevice d,VkQueue queue,std::uint32_t family,const void* bytes,VkDeviceSize size){
    // Library-only callers can omit the queue; Android supplies one and stages once.
    if(!queue){auto out=buffer(p,d,size,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        try{write(d,out,bytes,size);return out;}catch(...){destroy(d,out);throw;}}
    Buffer out{},staging{};VkCommandPool pool{};VkFence fence{};bool submitted=false;
    auto cleanup=[&]{if(fence)vkDestroyFence(d,fence,nullptr);if(pool)vkDestroyCommandPool(d,pool,nullptr);destroy(d,staging);};
    try {
        out.size=size;VkBufferCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;ci.size=size;
        ci.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT;ci.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateBuffer(d,&ci,nullptr,&out.handle),"static vertex buffer");VkMemoryRequirements req{};vkGetBufferMemoryRequirements(d,out.handle,&req);
        VkMemoryAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;ai.allocationSize=req.size;
        ai.memoryTypeIndex=memoryType(p,req.memoryTypeBits,0,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(d,&ai,nullptr,&out.memory),"static vertex memory");check(vkBindBufferMemory(d,out.handle,out.memory,0),"static vertex bind");
        staging=buffer(p,d,size,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);write(d,staging,bytes,size);
        VkCommandPoolCreateInfo pc{};pc.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;pc.queueFamilyIndex=family;pc.flags=VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        check(vkCreateCommandPool(d,&pc,nullptr,&pool),"static upload pool");VkCommandBufferAllocateInfo ca{};ca.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ca.commandPool=pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;
        VkCommandBuffer cmd{};check(vkAllocateCommandBuffers(d,&ca,&cmd),"static upload command");VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(cmd,&bi),"static upload begin");VkBufferCopy copy{0,0,size};vkCmdCopyBuffer(cmd,staging.handle,out.handle,1,&copy);
        VkBufferMemoryBarrier barrier{};barrier.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.buffer=out.handle;barrier.size=VK_WHOLE_SIZE;
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,0,0,nullptr,1,&barrier,0,nullptr);
        check(vkEndCommandBuffer(cmd),"static upload end");VkFenceCreateInfo fc{};fc.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;check(vkCreateFence(d,&fc,nullptr,&fence),"static upload fence");
        VkSubmitInfo si{};si.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;si.commandBufferCount=1;si.pCommandBuffers=&cmd;check(vkQueueSubmit(queue,1,&si,fence),"static upload submit");submitted=true;
        check(vkWaitForFences(d,1,&fence,VK_TRUE,5000000000ULL),"static upload wait");cleanup();return out;
    }catch(...){if(submitted)vkQueueWaitIdle(queue);cleanup();destroy(d,out);throw;}
}
struct Image{VkImage handle{};VkDeviceMemory memory{};VkImageView view{};};
inline void destroy(VkDevice d,Image& i){if(i.view)vkDestroyImageView(d,i.view,nullptr);
    if(i.handle)vkDestroyImage(d,i.handle,nullptr);
    if(i.memory)vkFreeMemory(d,i.memory,nullptr);
    i={};}
inline bool supported(VkPhysicalDevice p,VkFormat f,VkImageUsageFlags usage,std::uint32_t w,std::uint32_t h,std::uint32_t levels=1){
    VkImageFormatProperties ip{};auto r=vkGetPhysicalDeviceImageFormatProperties(p,f,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,usage,0,&ip);
    if(r==VK_ERROR_FORMAT_NOT_SUPPORTED)return false;
    check(r,"arena image format");return w<=ip.maxExtent.width&&h<=ip.maxExtent.height&&levels<=ip.maxMipLevels&&(ip.sampleCounts&VK_SAMPLE_COUNT_1_BIT);
}
inline Image image(VkPhysicalDevice p,VkDevice d,VkFormat f,VkExtent2D e,std::uint32_t levels,VkImageUsageFlags usage,VkImageAspectFlags aspect){
    Image i{};try{
        VkImageCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;ci.imageType=VK_IMAGE_TYPE_2D;ci.format=f;ci.extent={e.width,e.height,1};ci.mipLevels=levels;ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=usage;ci.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateImage(d,&ci,nullptr,&i.handle),"arena image");VkMemoryRequirements req{};vkGetImageMemoryRequirements(d,i.handle,&req);
        VkMemoryAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;ai.allocationSize=req.size;ai.memoryTypeIndex=memoryType(p,req.memoryTypeBits,0,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(d,&ai,nullptr,&i.memory),"arena image memory");
    check(vkBindImageMemory(d,i.handle,i.memory,0),"arena image bind");
        VkImageViewCreateInfo vi{};vi.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;vi.image=i.handle;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=f;vi.subresourceRange={aspect,0,levels,0,1};
        check(vkCreateImageView(d,&vi,nullptr,&i.view),"arena image view");return i;
    }catch(...){destroy(d,i);
    throw;}
}
inline VkShaderModule shader(VkDevice d,const std::vector<std::uint32_t>& spv){
    if(spv.size()<5||spv[0]!=0x07230203||spv[1]!=0x10000)throw std::invalid_argument("Arena shaders require SPIR-V 1.0");
    VkShaderModuleCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;ci.codeSize=spv.size()*4;ci.pCode=spv.data();VkShaderModule out{};
    check(vkCreateShaderModule(d,&ci,nullptr,&out),"arena shader");return out;
}
inline void viewport(VkCommandBuffer cmd,VkExtent2D e){VkViewport v{0,0,float(e.width),float(e.height),0,1};VkRect2D s{{0,0},e};vkCmdSetViewport(cmd,0,1,&v);vkCmdSetScissor(cmd,0,1,&s);}
inline void validateExtent(VkPhysicalDevice p,VkExtent2D e){VkPhysicalDeviceProperties pp{};vkGetPhysicalDeviceProperties(p,&pp);const auto& l=pp.limits;
    if(!e.width||!e.height||e.width>l.maxViewportDimensions[0]||e.height>l.maxViewportDimensions[1]||e.width>l.maxFramebufferWidth||e.height>l.maxFramebufferHeight||l.viewportBoundsRange[0]>0||float(e.width)>l.viewportBoundsRange[1]||float(e.height)>l.viewportBoundsRange[1])throw std::invalid_argument("Arena target exceeds device viewport/framebuffer limits");}
}
