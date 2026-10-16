// NDK command-line offscreen smoke test. No Android Activity or swapchain needed.
#include "../app/src/main/cpp/vulkan/src/arena_gpu.hpp"
#include "../app/src/main/cpp/vulkan/shaders/embedded.h"
#include "../app/src/main/cpp/vulkan_floor.h"
#include "../app/src/main/cpp/scene_instance_writer.h"
#include <iostream>
#include <cassert>
using namespace vk3d::arena;
using namespace vk3d::arena::gpu;
int main(){
    VkInstance instanceHandle{};VkApplicationInfo app{};app.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;app.apiVersion=VK_API_VERSION_1_0;
    VkInstanceCreateInfo ic{};ic.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;ic.pApplicationInfo=&app;check(vkCreateInstance(&ic,nullptr,&instanceHandle),"test instance");
    uint32_t count=0;check(vkEnumeratePhysicalDevices(instanceHandle,&count,nullptr),"test devices");assert(count);
    std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instanceHandle,&count,devices.data()),"test devices");auto p=devices.front();
    VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(p,&properties);
    std::cout<<"GPU: "<<properties.deviceName<<"\n";
    vkGetPhysicalDeviceQueueFamilyProperties(p,&count,nullptr);std::vector<VkQueueFamilyProperties> qs(count);vkGetPhysicalDeviceQueueFamilyProperties(p,&count,qs.data());
    uint32_t family=0;while(family<count&&!(qs[family].queueFlags&VK_QUEUE_GRAPHICS_BIT))++family;assert(family<count);
    float priority=1;VkDeviceQueueCreateInfo q{};q.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
    VkPhysicalDeviceFeatures features{};vkGetPhysicalDeviceFeatures(p,&features);VkPhysicalDeviceFeatures enabled{};
    enabled.textureCompressionASTC_LDR=features.textureCompressionASTC_LDR;enabled.textureCompressionETC2=features.textureCompressionETC2;enabled.samplerAnisotropy=features.samplerAnisotropy;
    VkDeviceCreateInfo dc{};dc.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;dc.queueCreateInfoCount=1;dc.pQueueCreateInfos=&q;dc.pEnabledFeatures=&enabled;
    VkDevice d{};check(vkCreateDevice(p,&dc,nullptr,&d),"test device");VkQueue queue{};vkGetDeviceQueue(d,family,0,&queue);
    VkCommandPoolCreateInfo pc{};pc.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;pc.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;pc.queueFamilyIndex=family;
    VkCommandPool pool{};check(vkCreateCommandPool(d,&pc,nullptr,&pool),"test pool");VkCommandBufferAllocateInfo ca{};ca.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ca.commandPool=pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;
    VkCommandBuffer cmd{};check(vkAllocateCommandBuffers(d,&ca,&cmd),"test command");VkFenceCreateInfo fc{};fc.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;fc.flags=VK_FENCE_CREATE_SIGNALED_BIT;VkFence fence{};check(vkCreateFence(d,&fc,nullptr,&fence),"test fence");
    auto begin=[&]{check(vkResetCommandBuffer(cmd,0),"test reset");VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;check(vkBeginCommandBuffer(cmd,&bi),"test begin");};
    auto submit=[&]{check(vkEndCommandBuffer(cmd),"test end");check(vkResetFences(d,1,&fence),"test reset fence");VkSubmitInfo si{};si.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;si.commandBufferCount=1;si.pCommandBuffers=&cmd;check(vkQueueSubmit(queue,1,&si,fence),"test submit");check(vkWaitForFences(d,1,&fence,VK_TRUE,10000000000ULL),"test wait");};
    VkFormat depthFormat=VK_FORMAT_UNDEFINED;for(auto f:{VK_FORMAT_D32_SFLOAT,VK_FORMAT_D16_UNORM})if(supported(p,f,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,128,128)){depthFormat=f;break;}assert(depthFormat);
    VkAttachmentDescription attachments[2]{};attachments[0].format=VK_FORMAT_R8G8B8A8_UNORM;attachments[1].format=depthFormat;
    for(auto& a:attachments){a.samples=VK_SAMPLE_COUNT_1_BIT;a.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;a.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;}
    attachments[0].finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference colorRef{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depthRef{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&colorRef;sub.pDepthStencilAttachment=&depthRef;
    VkSubpassDependency dep{};dep.srcSubpass=0;dep.dstSubpass=VK_SUBPASS_EXTERNAL;dep.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;dep.dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT;dep.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;dep.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    VkRenderPassCreateInfo rp{};rp.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;rp.attachmentCount=2;rp.pAttachments=attachments;rp.subpassCount=1;rp.pSubpasses=&sub;rp.dependencyCount=1;rp.pDependencies=&dep;
    VkRenderPass pass{};check(vkCreateRenderPass(d,&rp,nullptr,&pass),"test pass");
    auto color=image(p,d,VK_FORMAT_R8G8B8A8_UNORM,{128,128},1,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT);
    auto depth=image(p,d,depthFormat,{128,128},1,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_IMAGE_ASPECT_DEPTH_BIT);
    VkImageView views[]={color.view,depth.view};VkFramebufferCreateInfo fi{};fi.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;fi.renderPass=pass;fi.attachmentCount=2;fi.pAttachments=views;fi.width=fi.height=128;fi.layers=1;VkFramebuffer fb{};check(vkCreateFramebuffer(d,&fi,nullptr,&fb),"test framebuffer");
    auto readback=buffer(p,d,128*128*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    {
        TextureStore textures(p,d,enabled.textureCompressionASTC_LDR,enabled.textureCompressionETC2,enabled.samplerAnisotropy,8);
        graphics::vk::FloorAsset asset(0);
        for(auto preferred:{TextureFamily::astc,TextureFamily::etc2,TextureFamily::rgba8}){
            asset.asset.preferred=preferred;begin();const auto id=textures.upload(cmd,asset.asset);submit();textures.releaseStaging();
            const auto expected=preferred==TextureFamily::astc&&!enabled.textureCompressionASTC_LDR?(enabled.textureCompressionETC2?TextureFamily::etc2:TextureFamily::rgba8):preferred==TextureFamily::etc2&&!enabled.textureCompressionETC2?(enabled.textureCompressionASTC_LDR?TextureFamily::astc:TextureFamily::rgba8):preferred;
            assert(textures.family(id)==expected);
            for(uint32_t shadowSize:{0u,128u}){
                ScenePipeline pipeline(PipelineConfig{{p,d,pass,bool(enabled.textureCompressionASTC_LDR),queue,family},2,64,shadowSize},Shaders{arena_spv::vertex(),arena_spv::fragment()});
                std::vector<Instance> batch;batch.reserve(64);const auto capacity=batch.capacity();
                const std::vector<Instance> fixed={vk3d::arena::instance(Box{-4,-.18f,-4,8,.18f,8,1,1,1,0,true,0}),
                    vk3d::arena::instance(Box{-.3f,0,-.3f,.6f,1,.6f,1,.2f,.1f})};
                std::vector<uint8_t> reference;
                VkRenderPass compatible{};check(vkCreateRenderPass(d,&rp,nullptr,&compatible),"compatible replacement pass");
                for(uint32_t frame=0;frame<4;++frame){
                    if(frame==2)pipeline.setStaticInstances(fixed);
                    if(frame==3)pipeline.useCompatibleRenderPass(compatible);
                    batch.clear();if(frame<2)batch.insert(batch.end(),fixed.begin(),fixed.end());
                    if(frame<2)batch.push_back(vk3d::arena::instance(Box{1,0,.5f,.3f,.7f,.3f,.1f,.5f,1}));
                    else {SceneInstanceWriter direct(batch,64);direct.push_back(Box{1,0,.5f,.3f,.7f,.3f,.1f,.5f,1});}
                    begin();pipeline.record(cmd,frame%2,fence,fb,{128,128},frameData(128,128,0,0),batch,textures.sampled(id),{0,0,0,1});assert(batch.capacity()==capacity);
                    VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={128,128,1};vkCmdCopyImageToBuffer(cmd,color.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.handle,1,&copy);
                    VkBufferMemoryBarrier barrier{};barrier.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.buffer=readback.handle;barrier.size=VK_WHOLE_SIZE;
                    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&barrier,0,nullptr);submit();
                    if(!readback.coherent){VkMappedMemoryRange range{};range.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;range.memory=readback.memory;range.size=VK_WHOLE_SIZE;check(vkInvalidateMappedMemoryRanges(d,1,&range),"test invalidate");}
                    const auto* bytes=static_cast<const uint8_t*>(readback.mapped);size_t lit=0;for(size_t i=0;i<128*128;++i)if(bytes[i*4]>20||bytes[i*4+1]>20||bytes[i*4+2]>20)++lit;
                    assert(lit>1000);assert(bytes[(64*128+64)*4+3]==255);
                    if(frame==0)reference.assign(bytes,bytes+128*128*4);
                    else assert(std::equal(reference.begin(),reference.end(),bytes));
                }
                vkDestroyRenderPass(d,compatible,nullptr);
                std::cout<<"family="<<int(textures.family(id))<<" shadows="<<pipeline.shadowsAvailable()<<" static/dynamic pixel equality, compatible pass and slot reuse passed\n";
            }
        }
    }
    destroy(d,readback);vkDestroyFramebuffer(d,fb,nullptr);destroy(d,color);destroy(d,depth);vkDestroyRenderPass(d,pass,nullptr);vkDestroyFence(d,fence,nullptr);vkDestroyCommandPool(d,pool,nullptr);vkDestroyDevice(d,nullptr);vkDestroyInstance(instanceHandle,nullptr);
    std::cout<<"Vulkan GPU smoke tests passed\n";
}
