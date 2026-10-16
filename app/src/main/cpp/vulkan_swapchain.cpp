#include "vulkan_swapchain.h"
#include <android/log.h>
#include <algorithm>
#include <stdexcept>

namespace graphics::vk {
Swapchain::Swapchain(Context& c,ANativeWindow* window):context(c){try{
    VkSurfaceCapabilitiesKHR caps{};check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(c.physical,c.surface,&caps),"surface capabilities");
    if(!(caps.supportedUsageFlags&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))throw std::runtime_error("Surface cannot be a color attachment");
    uint32_t count=0;check(vkGetPhysicalDeviceSurfaceFormatsKHR(c.physical,c.surface,&count,nullptr),"surface formats");
    if(!count)throw std::runtime_error("Surface has no formats");
    std::vector<VkSurfaceFormatKHR> formats(count);check(vkGetPhysicalDeviceSurfaceFormatsKHR(c.physical,c.surface,&count,formats.data()),"surface formats");
    auto chosen=formats.front();
    for(const auto& f:formats)if((f.format==VK_FORMAT_R8G8B8A8_UNORM||f.format==VK_FORMAT_B8G8R8A8_UNORM)&&f.colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){chosen=f;break;}
    if(chosen.format==VK_FORMAT_UNDEFINED)chosen.format=VK_FORMAT_R8G8B8A8_UNORM;
    colorFormat=chosen.format;
    extent=caps.currentExtent;
    if(extent.width==UINT32_MAX){extent.width=std::clamp(uint32_t(std::max(1,ANativeWindow_getWidth(window))),caps.minImageExtent.width,caps.maxImageExtent.width);
        extent.height=std::clamp(uint32_t(std::max(1,ANativeWindow_getHeight(window))),caps.minImageExtent.height,caps.maxImageExtent.height);}
    if(!extent.width||!extent.height)throw std::runtime_error("Surface has zero extent");
    geometry=surfaceGeometry(extent,caps.currentTransform);extent=geometry.image;
    uint32_t imageCount=std::max(2u,caps.minImageCount+1);if(caps.maxImageCount)imageCount=std::min(imageCount,caps.maxImageCount);
    VkCompositeAlphaFlagBitsKHR alpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    for(auto a:{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR})if(caps.supportedCompositeAlpha&a){alpha=a;break;}
    VkSwapchainCreateInfoKHR ci{};ci.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;ci.surface=c.surface;
    ci.minImageCount=imageCount;ci.imageFormat=chosen.format;ci.imageColorSpace=chosen.colorSpace;ci.imageExtent=extent;ci.imageArrayLayers=1;
    ci.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;ci.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
    // Match Android's transform; shader rotates clip coordinates into identity images.
    ci.preTransform=caps.currentTransform;
    ci.compositeAlpha=alpha;ci.presentMode=VK_PRESENT_MODE_FIFO_KHR;ci.clipped=VK_TRUE;
    check(vkCreateSwapchainKHR(c.device,&ci,nullptr,&handle),"swapchain");
    for(auto f:{VK_FORMAT_D32_SFLOAT,VK_FORMAT_D16_UNORM}){VkFormatProperties p{};vkGetPhysicalDeviceFormatProperties(c.physical,f,&p);
        if((p.optimalTilingFeatures&VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)&&vk3d::arena::gpu::supported(c.physical,f,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,extent.width,extent.height)){depthFormat=f;break;}}
    if(!depthFormat)throw std::runtime_error("No depth attachment format");
    VkAttachmentDescription attachments[2]{};
    attachments[0].format=colorFormat;attachments[0].samples=VK_SAMPLE_COUNT_1_BIT;attachments[0].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachments[0].storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachments[0].stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachments[0].finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[1].format=depthFormat;attachments[1].samples=VK_SAMPLE_COUNT_1_BIT;attachments[1].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachments[1].storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachments[1].stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depth{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;sub.pDepthStencilAttachment=&depth;
    VkSubpassDependency deps[2]{};deps[0].srcSubpass=VK_SUBPASS_EXTERNAL;deps[0].dstSubpass=0;
    deps[0].srcStageMask=deps[0].dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[0].srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;deps[0].dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].srcSubpass=0;deps[1].dstSubpass=VK_SUBPASS_EXTERNAL;deps[1].srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;deps[1].dstStageMask=VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;deps[1].srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo rp{};rp.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;rp.attachmentCount=2;rp.pAttachments=attachments;rp.subpassCount=1;rp.pSubpasses=&sub;rp.dependencyCount=2;rp.pDependencies=deps;
    check(vkCreateRenderPass(c.device,&rp,nullptr,&pass),"scene renderpass");
    check(vkGetSwapchainImagesKHR(c.device,handle,&count,nullptr),"swapchain images");std::vector<VkImage> images(count);check(vkGetSwapchainImagesKHR(c.device,handle,&count,images.data()),"swapchain images");
    targets.resize(count);
    for(uint32_t i=0;i<count;++i){auto& t=targets[i];t.image=images[i];
        VkImageViewCreateInfo vi{};vi.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;vi.image=t.image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=colorFormat;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        check(vkCreateImageView(c.device,&vi,nullptr,&t.view),"swapchain view");
        t.depth=vk3d::arena::gpu::image(c.physical,c.device,depthFormat,extent,1,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_IMAGE_ASPECT_DEPTH_BIT);
        VkImageView views[]={t.view,t.depth.view};VkFramebufferCreateInfo fi{};fi.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;fi.renderPass=pass;fi.attachmentCount=2;fi.pAttachments=views;fi.width=extent.width;fi.height=extent.height;fi.layers=1;
        check(vkCreateFramebuffer(c.device,&fi,nullptr,&t.framebuffer),"scene framebuffer");
        VkSemaphoreCreateInfo si{};si.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;check(vkCreateSemaphore(c.device,&si,nullptr,&t.rendered),"present semaphore");
    }
    __android_log_print(ANDROID_LOG_INFO,"ArenaVulkan","Swapchain %ux%u images=%u FIFO preTransform=%u",extent.width,extent.height,count,unsigned(caps.currentTransform));
}catch(...){release();throw;}}
Swapchain::~Swapchain(){release();}
void Swapchain::release() noexcept {
    auto d=context.device;
    for(auto& t:targets){if(t.framebuffer)vkDestroyFramebuffer(d,t.framebuffer,nullptr);vk3d::arena::gpu::destroy(d,t.depth);if(t.view)vkDestroyImageView(d,t.view,nullptr);if(t.rendered)vkDestroySemaphore(d,t.rendered,nullptr);}
    if(pass)vkDestroyRenderPass(d,pass,nullptr);
    if(handle)vkDestroySwapchainKHR(d,handle,nullptr);
}
}
