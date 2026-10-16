#include "arena_gpu.hpp"
#include "arena_instances.hpp"
#include <cmath>
#include <limits>
namespace vk3d::arena {
using namespace gpu;
struct ScenePipeline::Impl {
    PipelineConfig config;VkDescriptorSetLayout descriptors{};VkDescriptorPool pool{};
    VkPipelineLayout layout{};VkPipeline scene{},shadow{};VkRenderPass shadowPass{};
    VkSampler shadowSampler{};Buffer cube{};VkFormat depthFormat=VK_FORMAT_UNDEFINED;VkPhysicalDeviceProperties properties{};
    Buffer staticInstances{};std::uint32_t staticCount=0,staticCasters=0;
    VkDeviceSize instanceOffset=0;
    struct Slot {Buffer storage;Image depth;VkFramebuffer framebuffer{};VkDescriptorSet set{};SampledImage boundFloor{};};
    std::vector<Slot> slots;
    VkDevice device()const{return config.context.device;}
    ~Impl(){auto d=device();for(auto& s:slots){if(s.framebuffer)vkDestroyFramebuffer(d,s.framebuffer,nullptr);destroy(d,s.depth);destroy(d,s.storage);}
        destroy(d,staticInstances);
        destroy(d,cube);
    if(scene)vkDestroyPipeline(d,scene,nullptr);
    if(shadow)vkDestroyPipeline(d,shadow,nullptr);
        if(shadowSampler)vkDestroySampler(d,shadowSampler,nullptr);
    if(shadowPass)vkDestroyRenderPass(d,shadowPass,nullptr);
        if(pool)vkDestroyDescriptorPool(d,pool,nullptr);
    if(layout)vkDestroyPipelineLayout(d,layout,nullptr);
    if(descriptors)vkDestroyDescriptorSetLayout(d,descriptors,nullptr);}
    void createShadowPass(){
        VkAttachmentDescription a{};a.format=depthFormat;a.samples=VK_SAMPLE_COUNT_1_BIT;a.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;a.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;a.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        VkAttachmentReference ref{0,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.pDepthStencilAttachment=&ref;
        // Both directions matter when a slot is reused. Prior sampling -> depth write -> current sampling.
        VkSubpassDependency deps[2]{};
        deps[0].srcSubpass=VK_SUBPASS_EXTERNAL;deps[0].dstSubpass=0;deps[0].srcStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;deps[0].dstStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;deps[0].srcAccessMask=VK_ACCESS_SHADER_READ_BIT;deps[0].dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        deps[1].srcSubpass=0;deps[1].dstSubpass=VK_SUBPASS_EXTERNAL;deps[1].srcStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;deps[1].dstStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;deps[1].srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;deps[1].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        VkRenderPassCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;ci.attachmentCount=1;ci.pAttachments=&a;ci.subpassCount=1;ci.pSubpasses=&sub;ci.dependencyCount=2;ci.pDependencies=deps;
        check(vkCreateRenderPass(device(),&ci,nullptr,&shadowPass),"arena shadow renderpass");
    }
    VkPipeline createPipeline(VkShaderModule vs,VkShaderModule fs,bool depthOnly){
        VkPipelineShaderStageCreateInfo stages[2]{};for(auto& s:stages){s.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;s.pName="main";}
        stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vs;stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=fs;
        VkVertexInputBindingDescription binds[]={{0,24,VK_VERTEX_INPUT_RATE_VERTEX},{1,sizeof(Instance),VK_VERTEX_INPUT_RATE_INSTANCE}};
        VkVertexInputAttributeDescription attrs[]={{0,0,VK_FORMAT_R32G32B32_SFLOAT,0},{1,0,VK_FORMAT_R32G32B32_SFLOAT,12},
            {2,1,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Instance,origin)},{3,1,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Instance,extent)},{4,1,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Instance,color)},
            {5,1,VK_FORMAT_R32G32_SFLOAT,offsetof(Instance,rotation)},{6,1,VK_FORMAT_R32G32_SFLOAT,offsetof(Instance,material)}};
        VkPipelineVertexInputStateCreateInfo vi{};vi.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;vi.vertexBindingDescriptionCount=2;vi.pVertexBindingDescriptions=binds;vi.vertexAttributeDescriptionCount=7;vi.pVertexAttributeDescriptions=attrs;
        VkPipelineInputAssemblyStateCreateInfo ia{};ia.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo vp{};vp.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;vp.viewportCount=vp.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo rs{};rs.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=VK_CULL_MODE_NONE;rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;rs.lineWidth=1;rs.depthBiasEnable=depthOnly;rs.depthBiasConstantFactor=4;rs.depthBiasSlopeFactor=2;
        VkPipelineMultisampleStateCreateInfo ms{};ms.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo ds{};ds.sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;ds.depthTestEnable=ds.depthWriteEnable=VK_TRUE;ds.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;
        VkPipelineColorBlendAttachmentState ba{};ba.colorWriteMask=15;
        VkPipelineColorBlendStateCreateInfo bs{};bs.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;bs.attachmentCount=depthOnly?0:1;bs.pAttachments=depthOnly?nullptr:&ba;
        VkDynamicState states[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo dy{};dy.sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;dy.dynamicStateCount=2;dy.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;ci.stageCount=depthOnly?1:2;ci.pStages=stages;ci.pVertexInputState=&vi;ci.pInputAssemblyState=&ia;ci.pViewportState=&vp;ci.pRasterizationState=&rs;ci.pMultisampleState=&ms;ci.pDepthStencilState=&ds;ci.pColorBlendState=&bs;ci.pDynamicState=&dy;ci.layout=layout;ci.renderPass=depthOnly?shadowPass:config.context.renderPass;
        VkPipeline result{};const auto status=vkCreateGraphicsPipelines(device(),VK_NULL_HANDLE,1,&ci,nullptr,&result);
        if(status!=VK_SUCCESS){if(result)vkDestroyPipeline(device(),result,nullptr);
    check(status,"arena graphics pipeline");}return result;
    }
};
ScenePipeline::ScenePipeline(const PipelineConfig& cfg,const Shaders& shaders):impl(new Impl{}){
    auto& s=*impl;s.config=cfg;const auto& c=cfg.context;auto d=c.device;
    if(!d||!c.physical||!c.renderPass||!cfg.frameSlots||cfg.frameSlots>16||!cfg.maxInstances)throw std::invalid_argument("Invalid arena pipeline context/capacity");
    VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(c.physical,&props);s.properties=props;
    if(props.limits.maxUniformBufferRange<sizeof(FrameData)||props.limits.maxVertexInputAttributes<7||props.limits.maxVertexInputBindings<2||props.limits.maxVertexInputBindingStride<sizeof(Instance))throw std::runtime_error("Arena vertex/UBO device limits unavailable");
    VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
        {1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{2,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo dl{};dl.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;dl.bindingCount=3;dl.pBindings=bindings;
    check(vkCreateDescriptorSetLayout(d,&dl,nullptr,&s.descriptors),"arena descriptor layout");
    VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT,0,4};VkPipelineLayoutCreateInfo pl{};pl.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;pl.setLayoutCount=1;pl.pSetLayouts=&s.descriptors;pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&push;
    check(vkCreatePipelineLayout(d,&pl,nullptr,&s.layout),"arena pipeline layout");
    const auto shadowUsage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    if(cfg.shadowSize){
        // Unsupported depth attachment+sampling disables shadows; allocation failures propagate.
        for(auto f:{VK_FORMAT_D32_SFLOAT,VK_FORMAT_D16_UNORM}){
            VkFormatProperties fp{};vkGetPhysicalDeviceFormatProperties(c.physical,f,&fp);
            const auto required=VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
            if((fp.optimalTilingFeatures&required)==required&&supported(c.physical,f,shadowUsage,cfg.shadowSize,cfg.shadowSize)){
                validateExtent(c.physical,{cfg.shadowSize,cfg.shadowSize});s.depthFormat=f;break;
            }
        }
    }
    if(s.depthFormat!=VK_FORMAT_UNDEFINED){
        s.createShadowPass();VkSamplerCreateInfo si{};si.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;si.magFilter=si.minFilter=VK_FILTER_NEAREST;si.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        check(vkCreateSampler(d,&si,nullptr,&s.shadowSampler),"arena shadow sampler");
    }
    VkShaderModule vs{},fs{};try{vs=shader(d,shaders.vertex);fs=shader(d,shaders.fragment);s.scene=s.createPipeline(vs,fs,false);
    if(s.shadowPass)s.shadow=s.createPipeline(vs,fs,true);}
    catch(...){if(vs)vkDestroyShaderModule(d,vs,nullptr);
    if(fs)vkDestroyShaderModule(d,fs,nullptr);
    throw;}
    vkDestroyShaderModule(d,vs,nullptr);vkDestroyShaderModule(d,fs,nullptr);
    struct Vertex{float p[3],n[3];};std::vector<Vertex> cube;
    auto face=[&](float nx,float ny,float nz,std::initializer_list<float> points){auto p=points.begin();Vertex corners[4]{};for(auto& v:corners){for(float& coordinate:v.p)coordinate=*p++;v.n[0]=nx;v.n[1]=ny;v.n[2]=nz;}for(int i:{0,1,2,2,1,3})cube.push_back(corners[i]);};
    face(0,1,0,{0,1,0,0,1,1,1,1,0,1,1,1});face(0,-1,0,{0,0,1,0,0,0,1,0,1,1,0,0});
    face(0,0,1,{0,0,1,1,0,1,0,1,1,1,1,1});face(0,0,-1,{1,0,0,0,0,0,1,1,0,0,1,0});
    face(1,0,0,{1,0,1,1,0,0,1,1,1,1,1,0});face(-1,0,0,{0,0,0,0,0,1,0,1,0,0,1,1});
    s.cube=staticVertexBuffer(c.physical,d,c.uploadQueue,c.uploadQueueFamily,cube.data(),cube.size()*sizeof(Vertex));
    VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,cfg.frameSlots},{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,cfg.frameSlots*2}};
    VkDescriptorPoolCreateInfo dp{};dp.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;dp.maxSets=cfg.frameSlots;dp.poolSizeCount=2;dp.pPoolSizes=sizes;
    check(vkCreateDescriptorPool(d,&dp,nullptr,&s.pool),"arena descriptor pool");
    s.slots.resize(cfg.frameSlots);
    for(auto& slot:s.slots){
        // UBO at offset zero; vertex stream follows on a device/CPU-safe boundary.
        const auto alignment=std::max<VkDeviceSize>(16,props.limits.minUniformBufferOffsetAlignment);
        s.instanceOffset=((sizeof(FrameData)+alignment-1)/alignment)*alignment;
        slot.storage=buffer(c.physical,d,s.instanceOffset+VkDeviceSize(cfg.maxInstances)*sizeof(Instance),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        if(s.shadowPass){slot.depth=image(c.physical,d,s.depthFormat,{cfg.shadowSize,cfg.shadowSize},1,shadowUsage,VK_IMAGE_ASPECT_DEPTH_BIT);
            VkFramebufferCreateInfo fi{};fi.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;fi.renderPass=s.shadowPass;fi.attachmentCount=1;fi.pAttachments=&slot.depth.view;fi.width=fi.height=cfg.shadowSize;fi.layers=1;
    check(vkCreateFramebuffer(d,&fi,nullptr,&slot.framebuffer),"arena shadow framebuffer");}
        VkDescriptorSetAllocateInfo da{};da.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;da.descriptorPool=s.pool;da.descriptorSetCount=1;da.pSetLayouts=&s.descriptors;
    check(vkAllocateDescriptorSets(d,&da,&slot.set),"arena frame descriptor");
    }
}
ScenePipeline::~ScenePipeline()=default;
bool ScenePipeline::shadowsAvailable()const noexcept{return impl->shadowPass!=VK_NULL_HANDLE;}
void ScenePipeline::useCompatibleRenderPass(VkRenderPass pass){
    if(!pass)throw std::invalid_argument("Missing compatible renderpass");
    impl->config.context.renderPass=pass;
}
void ScenePipeline::setStaticInstances(std::vector<Instance> instances){
    auto& s=*impl;
    if(instances.size()>s.config.maxInstances)throw std::length_error("Static instance capacity exceeded");
    detail::validateInstances(instances);
    const auto casters=detail::partitionCasters(instances);
    Buffer next{};
    if(!instances.empty())next=staticVertexBuffer(s.config.context.physical,s.device(),s.config.context.uploadQueue,
        s.config.context.uploadQueueFamily,instances.data(),instances.size()*sizeof(Instance));
    destroy(s.device(),s.staticInstances);s.staticInstances=next;
    s.staticCount=static_cast<std::uint32_t>(instances.size());s.staticCasters=casters;
}
void ScenePipeline::record(VkCommandBuffer cmd,std::uint32_t index,VkFence fence,VkFramebuffer target,VkExtent2D extent,
                           const FrameData& input,std::vector<Instance>& instances,SampledImage floor,std::array<float,4> color){
    auto& s=*impl;auto d=s.device();
    if(!cmd||!fence||!target||!floor.view||!floor.sampler)throw std::invalid_argument("Missing arena recording resources");
    auto& slot=s.slots.at(index);auto status=vkGetFenceStatus(d,fence);
    if(status==VK_NOT_READY)throw std::logic_error("Arena frame slot is still in flight");
    check(status,"arena slot fence");
    if(instances.size()+s.staticCount>s.config.maxInstances)throw std::length_error("Arena instance capacity exceeded");
    const auto& limits=s.properties.limits;
    if(!extent.width||!extent.height||extent.width>limits.maxViewportDimensions[0]||extent.height>limits.maxViewportDimensions[1]||extent.width>limits.maxFramebufferWidth||extent.height>limits.maxFramebufferHeight||float(extent.width)>limits.viewportBoundsRange[1]||float(extent.height)>limits.viewportBoundsRange[1])throw std::invalid_argument("Invalid scene extent");
    for(float x:input.projection)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite projection");
    for(float x:input.camera)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite camera");
    for(float x:color)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite clear color");
    auto& sorted=instances;
    detail::validateInstances(sorted);
    const auto casters=detail::partitionCasters(sorted);FrameData frame=input;frame.camera[2]=s.shadowPass?1.f:0.f;
    write(d,slot.storage,&frame,sizeof(frame));writeAt(d,slot.storage,sorted.data(),sorted.size()*sizeof(Instance),s.instanceOffset);
    if(slot.boundFloor.view!=floor.view||slot.boundFloor.sampler!=floor.sampler){
        VkDescriptorBufferInfo bi{slot.storage.handle,0,sizeof(FrameData)};
        VkDescriptorImageInfo images[]={{floor.sampler,floor.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {s.shadowPass?s.shadowSampler:floor.sampler,s.shadowPass?slot.depth.view:floor.view,s.shadowPass?VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
        VkWriteDescriptorSet writes[3]{};for(std::uint32_t i=0;i<3;i++){auto& w=writes[i];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=slot.set;w.dstBinding=i;w.descriptorCount=1;w.descriptorType=i?VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        if(i)w.pImageInfo=&images[i-1];else w.pBufferInfo=&bi;}
        vkUpdateDescriptorSets(d,3,writes,0,nullptr);slot.boundFloor=floor;
    }
    auto draw=[&](bool shadowPass){
        VkBuffer bufs[]={s.cube.handle,s.staticInstances.handle};VkDeviceSize offsets[]={0,0};
        const auto statics=shadowPass?s.staticCasters:s.staticCount;
        if(statics){vkCmdBindVertexBuffers(cmd,0,2,bufs,offsets);vkCmdDraw(cmd,36,statics,0,0);}
        const auto dynamics=shadowPass?casters:static_cast<std::uint32_t>(sorted.size());
        if(dynamics){bufs[1]=slot.storage.handle;offsets[1]=s.instanceOffset;
            vkCmdBindVertexBuffers(cmd,0,2,bufs,offsets);vkCmdDraw(cmd,36,dynamics,0,0);}
    };
    vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,s.layout,0,1,&slot.set,0,nullptr);
    if(s.shadowPass){VkClearValue clear{};clear.depthStencil={1,0};VkRenderPassBeginInfo rp{};rp.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;rp.renderPass=s.shadowPass;rp.framebuffer=slot.framebuffer;rp.renderArea={{0,0},{s.config.shadowSize,s.config.shadowSize}};rp.clearValueCount=1;rp.pClearValues=&clear;
        vkCmdBeginRenderPass(cmd,&rp,VK_SUBPASS_CONTENTS_INLINE);viewport(cmd,{s.config.shadowSize,s.config.shadowSize});vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,s.shadow);std::uint32_t pass=1;vkCmdPushConstants(cmd,s.layout,VK_SHADER_STAGE_VERTEX_BIT,0,4,&pass);
    draw(true);
    vkCmdEndRenderPass(cmd);}
    VkClearValue clears[2]{};std::copy(color.begin(),color.end(),clears[0].color.float32);clears[1].depthStencil={1,0};
    VkRenderPassBeginInfo rp{};rp.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;rp.renderPass=s.config.context.renderPass;rp.framebuffer=target;rp.renderArea={{0,0},extent};rp.clearValueCount=2;rp.pClearValues=clears;
    vkCmdBeginRenderPass(cmd,&rp,VK_SUBPASS_CONTENTS_INLINE);viewport(cmd,extent);vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,s.scene);std::uint32_t pass=0;vkCmdPushConstants(cmd,s.layout,VK_SHADER_STAGE_VERTEX_BIT,0,4,&pass);
    draw(false);
    vkCmdEndRenderPass(cmd);
}
}
