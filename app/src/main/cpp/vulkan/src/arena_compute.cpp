#include "arena_gpu.hpp"
namespace vk3d::arena {
using namespace gpu;
struct ComputeCopy::Impl {
    VkPhysicalDevice physical{};VkDevice device{};VkDescriptorSetLayout descriptors{};VkDescriptorPool pool{};VkPipelineLayout layout{};VkPipeline pipeline{};std::vector<VkDescriptorSet> sets;
    ~Impl(){if(pipeline)vkDestroyPipeline(device,pipeline,nullptr);if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(pool)vkDestroyDescriptorPool(device,pool,nullptr);if(descriptors)vkDestroyDescriptorSetLayout(device,descriptors,nullptr);}
};
ComputeCopy::ComputeCopy(VkPhysicalDevice p,VkDevice d,std::uint32_t family,std::uint32_t slots,const std::vector<std::uint32_t>& spv):impl(new Impl{}){
    auto& s=*impl;s.physical=p;s.device=d;
    if(!p||!d||!slots||slots>16)throw std::invalid_argument("Invalid compute copy context");
    std::uint32_t count=0;vkGetPhysicalDeviceQueueFamilyProperties(p,&count,nullptr);std::vector<VkQueueFamilyProperties> queues(count);vkGetPhysicalDeviceQueueFamilyProperties(p,&count,queues.data());
    if(family>=count||(queues[family].queueFlags&(VK_QUEUE_COMPUTE_BIT|VK_QUEUE_GRAPHICS_BIT))!=(VK_QUEUE_COMPUTE_BIT|VK_QUEUE_GRAPHICS_BIT))throw std::invalid_argument("Graphics+compute queue family required");
    VkFormatProperties fp{};vkGetPhysicalDeviceFormatProperties(p,VK_FORMAT_R8G8B8A8_UNORM,&fp);
    if(!(fp.optimalTilingFeatures&VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))throw std::runtime_error("RGBA8 storage image unavailable; use transfer/fragment copy");
    VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo dl{};dl.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;dl.bindingCount=2;dl.pBindings=bindings;check(vkCreateDescriptorSetLayout(d,&dl,nullptr,&s.descriptors),"compute copy descriptor layout");
    VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,8};VkPipelineLayoutCreateInfo pl{};pl.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;pl.setLayoutCount=1;pl.pSetLayouts=&s.descriptors;pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&push;check(vkCreatePipelineLayout(d,&pl,nullptr,&s.layout),"compute copy pipeline layout");
    auto module=shader(d,spv);VkComputePipelineCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;ci.layout=s.layout;ci.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;ci.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;ci.stage.module=module;ci.stage.pName="main";
    const auto result=vkCreateComputePipelines(d,VK_NULL_HANDLE,1,&ci,nullptr,&s.pipeline);vkDestroyShaderModule(d,module,nullptr);check(result,"compute copy pipeline");
    VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,slots},{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,slots}};
    VkDescriptorPoolCreateInfo dp{};dp.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;dp.maxSets=slots;dp.poolSizeCount=2;dp.pPoolSizes=sizes;check(vkCreateDescriptorPool(d,&dp,nullptr,&s.pool),"compute copy descriptor pool");
    s.sets.resize(slots);std::vector<VkDescriptorSetLayout> layouts(slots,s.descriptors);VkDescriptorSetAllocateInfo da{};da.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;da.descriptorPool=s.pool;da.descriptorSetCount=slots;da.pSetLayouts=layouts.data();check(vkAllocateDescriptorSets(d,&da,s.sets.data()),"compute copy descriptors");
}
ComputeCopy::~ComputeCopy()=default;
void ComputeCopy::record(VkCommandBuffer cmd,std::uint32_t slot,VkFence fence,CopyImage src,SampledImage view,CopyImage dst,VkImageView output,VkExtent2D e){
    auto& s=*impl;const auto set=s.sets.at(slot);
    if(!cmd||!fence||!src.image||!dst.image||src.image==dst.image||!view.view||!view.sampler||!output||!e.width||!e.height||e.width>INT32_MAX||e.height>INT32_MAX||!src.stage||!dst.stage||src.layout==VK_IMAGE_LAYOUT_UNDEFINED||src.layout==VK_IMAGE_LAYOUT_PREINITIALIZED)throw std::invalid_argument("Invalid compute copy resources");
    auto status=vkGetFenceStatus(s.device,fence);if(status==VK_NOT_READY)throw std::logic_error("Compute copy slot in flight");check(status,"compute copy slot fence");
    VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(s.physical,&props);const auto gx=(e.width+7)/8,gy=(e.height+7)/8;
    if(gx>props.limits.maxComputeWorkGroupCount[0]||gy>props.limits.maxComputeWorkGroupCount[1])throw std::invalid_argument("Compute dispatch extent exceeds limits");
    VkDescriptorImageInfo images[]={{view.sampler,view.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{VK_NULL_HANDLE,output,VK_IMAGE_LAYOUT_GENERAL}};
    VkWriteDescriptorSet writes[2]{};for(std::uint32_t i=0;i<2;i++){auto& w=writes[i];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=set;w.dstBinding=i;w.descriptorCount=1;w.descriptorType=i?VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;w.pImageInfo=&images[i];}vkUpdateDescriptorSets(s.device,2,writes,0,nullptr);
    VkImageMemoryBarrier b[2]{};for(auto& v:b){v.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;v.srcQueueFamilyIndex=v.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};}
    b[0].image=src.image;b[0].oldLayout=src.layout;b[0].newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;b[0].srcAccessMask=src.access;b[0].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    b[1].image=dst.image;b[1].oldLayout=dst.layout;b[1].newLayout=VK_IMAGE_LAYOUT_GENERAL;b[1].srcAccessMask=dst.access;b[1].dstAccessMask=VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd,src.stage|dst.stage,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,0,nullptr,2,b);
    vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,s.pipeline);vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,s.layout,0,1,&set,0,nullptr);const std::int32_t extent[]={static_cast<std::int32_t>(e.width),static_cast<std::int32_t>(e.height)};vkCmdPushConstants(cmd,s.layout,VK_SHADER_STAGE_COMPUTE_BIT,0,8,extent);vkCmdDispatch(cmd,gx,gy,1);
    b[0].oldLayout=b[0].newLayout;b[0].newLayout=src.layout;b[0].srcAccessMask=VK_ACCESS_SHADER_READ_BIT;b[0].dstAccessMask=src.access;
    b[1].oldLayout=b[1].newLayout;b[1].newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;b[1].srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;b[1].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,src.stage|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,2,b);
}
}
