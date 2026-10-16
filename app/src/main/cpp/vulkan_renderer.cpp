#include "vulkan_renderer.h"
#include "vulkan_context.h"
#include "vulkan_swapchain.h"
#include "vulkan_floor.h"
#include "vulkan/shaders/embedded.h"
#include <android/log.h>
#include <chrono>
#include <array>
#include <stdexcept>

using namespace vk3d::arena;
using graphics::vk::check;
struct VulkanRenderer::Impl {
    static constexpr uint32_t slotCount=2,maxInstances=4096;
    ANativeWindow* window{};
    std::unique_ptr<graphics::vk::Context> context;
    std::unique_ptr<graphics::vk::Swapchain> swapchain;
    std::unique_ptr<TextureStore> textures;
    std::unique_ptr<ScenePipeline> pipeline;
    std::vector<Instance> staticScene;
    VkFormat pipelineColor{},pipelineDepth{};
    int windowWidth=0,windowHeight=0;
    uint64_t generation=0;
    SampledImage floor{};
    VkCommandPool pool{};
    VkQueryPool queries{};
    struct Slot {VkCommandBuffer command{};VkFence fence{};VkSemaphore acquired{};
        std::vector<Instance> instances;bool timestamp=false;};
    std::array<Slot,slotCount> slots;
    uint32_t current=0,imageIndex=0;uint64_t frames=0;
    bool active=false,recreate=false,acquiredPending=false;
    float shake=0;std::array<float,4> clear{};
    std::chrono::steady_clock::time_point started=std::chrono::steady_clock::now();
    ~Impl(){release();}
    void createOutput(){
        auto& c=*context;
        swapchain=std::make_unique<graphics::vk::Swapchain>(c,window);
        PipelineConfig config{{c.physical,c.device,swapchain->pass,bool(c.enabled.textureCompressionASTC_LDR),c.queue,c.family},slotCount,maxInstances,1024};
        if(!pipeline||pipelineColor!=swapchain->colorFormat||pipelineDepth!=swapchain->depthFormat){
            pipeline=std::make_unique<ScenePipeline>(config,Shaders{arena_spv::vertex(),arena_spv::fragment()});
            if(!staticScene.empty())pipeline->setStaticInstances(staticScene);
            pipelineColor=swapchain->colorFormat;pipelineDepth=swapchain->depthFormat;
        } else pipeline->useCompatibleRenderPass(swapchain->pass);
        windowWidth=ANativeWindow_getWidth(window);windowHeight=ANativeWindow_getHeight(window);++generation;
        __android_log_print(ANDROID_LOG_INFO,"ArenaVulkan","Scene: 2 slots, persistent mapped buffers, %u instance capacity, shadows=%d",maxInstances,pipeline->shadowsAvailable());
    }
    void rebuild(){
        check(vkDeviceWaitIdle(context->device),"swapchain recreation idle");
        swapchain.reset();
        for(auto& s:slots){check(vkResetCommandBuffer(s.command,0),"reset retired commands");s.timestamp=false;}
        createOutput();recreate=false;
    }
    void release() noexcept {
        if(context&&context->device){auto d=context->device;
            // Pause can arrive after acquire but before frame(): consume that signal
            // before releasing the semaphore/window. Never wait twice after submit.
            if(acquiredPending){
                VkPipelineStageFlags stage=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                VkSubmitInfo drain{};drain.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
                drain.waitSemaphoreCount=1;drain.pWaitSemaphores=&slots[current].acquired;drain.pWaitDstStageMask=&stage;
                vkQueueSubmit(context->queue,1,&drain,VK_NULL_HANDLE);
            }
            vkDeviceWaitIdle(d);
            pipeline.reset();textures.reset();swapchain.reset();
            if(queries)vkDestroyQueryPool(d,queries,nullptr);
            if(pool)vkDestroyCommandPool(d,pool,nullptr);
            for(auto& s:slots){if(s.fence)vkDestroyFence(d,s.fence,nullptr);if(s.acquired)vkDestroySemaphore(d,s.acquired,nullptr);}
        }
        context.reset();
        if(window)ANativeWindow_release(window);
    }
};
VulkanRenderer::VulkanRenderer(ANativeWindow* window,int preference):impl(new Impl{}){
    if(!window)throw std::invalid_argument("Missing Android rendering surface");
    auto& s=*impl;
    s.window=window;ANativeWindow_acquire(window);
    s.context=std::make_unique<graphics::vk::Context>(window);auto& c=*s.context;
    VkCommandPoolCreateInfo pi{};pi.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;pi.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;pi.queueFamilyIndex=c.family;
    check(vkCreateCommandPool(c.device,&pi,nullptr,&s.pool),"frame command pool");
    for(auto& slot:s.slots){
        slot.instances.reserve(Impl::maxInstances);
        VkCommandBufferAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ai.commandPool=s.pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;
        check(vkAllocateCommandBuffers(c.device,&ai,&slot.command),"frame command buffer");
        VkFenceCreateInfo fi{};fi.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;fi.flags=VK_FENCE_CREATE_SIGNALED_BIT;check(vkCreateFence(c.device,&fi,nullptr,&slot.fence),"frame fence");
        VkSemaphoreCreateInfo si{};si.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;check(vkCreateSemaphore(c.device,&si,nullptr,&slot.acquired),"acquire semaphore");
    }
    s.textures=std::make_unique<TextureStore>(c.physical,c.device,c.enabled.textureCompressionASTC_LDR,c.enabled.textureCompressionETC2,c.enabled.samplerAnisotropy,8);
    graphics::vk::FloorAsset asset(preference);
    auto cmd=s.slots[0].command;VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(cmd,&bi),"begin texture upload");auto id=s.textures->upload(cmd,asset.asset);check(vkEndCommandBuffer(cmd),"end texture upload");
    auto fence=s.slots[0].fence;check(vkResetFences(c.device,1,&fence),"texture fence reset");
    VkSubmitInfo submit{};submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;check(vkQueueSubmit(c.queue,1,&submit,fence),"texture upload submit");
    check(vkWaitForFences(c.device,1,&fence,VK_TRUE,5000000000ULL),"texture upload completion");
    s.textures->releaseStaging();s.floor=s.textures->sampled(id);
    __android_log_print(ANDROID_LOG_INFO,"ArenaVulkan","Floor texture preferred=%d actual=%d (ASTC=1 ETC2=2 RGBA8=3)",preference,int(s.textures->family(id)));
    if(c.timestampBits){VkQueryPoolCreateInfo qi{};qi.sType=VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;qi.queryType=VK_QUERY_TYPE_TIMESTAMP;qi.queryCount=Impl::slotCount*2;check(vkCreateQueryPool(c.device,&qi,nullptr,&s.queries),"GPU timestamp pool");}
    s.createOutput();
}
VulkanRenderer::~VulkanRenderer()=default;
void VulkanRenderer::setCameraShake(float amount) noexcept{impl->shake=amount;}
unsigned long long VulkanRenderer::submittedFrames() const noexcept{return impl->frames;}
unsigned long long VulkanRenderer::outputGeneration() const noexcept{return impl->generation;}
void VulkanRenderer::setStaticScene(const std::vector<::Box>& boxes){
    auto& s=*impl;if(s.frames||s.active)throw std::logic_error("Static scene must be set before gameplay");
    s.staticScene.clear();s.staticScene.reserve(boxes.size());
    for(const auto& b:boxes)s.staticScene.push_back(instance(b));
    s.pipeline->setStaticInstances(s.staticScene);
}
bool VulkanRenderer::beginFrame(float r,float g,float b){
    auto& s=*impl;auto& c=*s.context;
    if(s.active)throw std::logic_error("Frame already active");
    if(ANativeWindow_getWidth(s.window)<=0||ANativeWindow_getHeight(s.window)<=0)return false;
    if(ANativeWindow_getWidth(s.window)!=s.windowWidth||ANativeWindow_getHeight(s.window)!=s.windowHeight)s.recreate=true;
    if(s.recreate)s.rebuild();
    auto& slot=s.slots[s.current];auto ready=vkWaitForFences(c.device,1,&slot.fence,VK_TRUE,50000000);
    if(ready==VK_TIMEOUT)return false;check(ready,"frame slot completion");
    if(slot.timestamp){double ms=0;if(pollTimestamp(c.device,s.queries,s.current*2,c.timestampBits,c.properties.limits.timestampPeriod,ms))
        __android_log_print(ANDROID_LOG_INFO,"ArenaVulkan","GPU scene %.3f ms",ms);
        slot.timestamp=false;}
    auto result=vkAcquireNextImageKHR(c.device,s.swapchain->handle,50000000,slot.acquired,VK_NULL_HANDLE,&s.imageIndex);
    if(result==VK_ERROR_OUT_OF_DATE_KHR){s.recreate=true;return false;}
    if(result==VK_TIMEOUT||result==VK_NOT_READY)return false;
    if(result==VK_SUBOPTIMAL_KHR)s.recreate=true;else check(result,"acquire swapchain image");
    s.acquiredPending=true;
    auto& target=s.swapchain->targets.at(s.imageIndex);
    // Slot fences cover CPU buffers; image fences cover the target's depth attachment.
    if(target.lastSubmission&&target.lastSubmission!=slot.fence)
        check(vkWaitForFences(c.device,1,&target.lastSubmission,VK_TRUE,1000000000ULL),"swapchain target completion");
    slot.instances.clear();s.clear={r,g,b,1};s.active=true;return true;
}
SceneInstanceWriter VulkanRenderer::dynamicScene(){
    auto& s=*impl;
    if(!s.active)throw std::logic_error("No active frame");
    return {s.slots[s.current].instances,Impl::maxInstances};
}
void VulkanRenderer::endFrame(){
    auto& s=*impl;auto& c=*s.context;if(!s.active)throw std::logic_error("No active frame");
    auto& slot=s.slots[s.current];auto& target=s.swapchain->targets[s.imageIndex];auto cmd=slot.command;
    check(vkResetCommandBuffer(cmd,0),"reset frame commands");VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(cmd,&bi),"begin frame commands");
    slot.timestamp=s.queries&&s.frames%120==0;if(slot.timestamp)beginTimestamp(cmd,s.queries,s.current*2);
    const auto elapsed=std::chrono::duration<float>(std::chrono::steady_clock::now()-s.started).count();
    auto e=s.swapchain->extent;
    const auto logical=s.swapchain->geometry.logical;
    auto frame=frameData(logical.width,logical.height,s.shake,elapsed);frame.camera[3]=s.swapchain->geometry.clipTransform;
    s.pipeline->record(cmd,s.current,slot.fence,target.framebuffer,e,frame,slot.instances,s.floor,s.clear);
    if(slot.timestamp)endTimestamp(cmd,s.queries,s.current*2);
    check(vkEndCommandBuffer(cmd),"end frame commands");
    const VkPipelineStageFlags wait=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si{};si.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;si.waitSemaphoreCount=1;si.pWaitSemaphores=&slot.acquired;si.pWaitDstStageMask=&wait;
    si.commandBufferCount=1;si.pCommandBuffers=&cmd;si.signalSemaphoreCount=1;si.pSignalSemaphores=&target.rendered;
    // Reset only once an image and a complete command buffer are ready to submit.
    check(vkResetFences(c.device,1,&slot.fence),"reset submit fence");check(vkQueueSubmit(c.queue,1,&si,slot.fence),"submit scene");s.acquiredPending=false;target.lastSubmission=slot.fence;
    VkPresentInfoKHR present{};present.sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;present.waitSemaphoreCount=1;present.pWaitSemaphores=&target.rendered;
    present.swapchainCount=1;present.pSwapchains=&s.swapchain->handle;present.pImageIndices=&s.imageIndex;
    auto result=vkQueuePresentKHR(c.queue,&present);
    if(result==VK_ERROR_OUT_OF_DATE_KHR||result==VK_SUBOPTIMAL_KHR)s.recreate=true;else check(result,"present scene");
    s.active=false;s.current=(s.current+1)%Impl::slotCount;++s.frames;
}
