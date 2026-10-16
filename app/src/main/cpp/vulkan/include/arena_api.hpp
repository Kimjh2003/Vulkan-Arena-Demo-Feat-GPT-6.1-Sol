#pragma once
#include "vulkan_common.hpp"
#include "arena_scene_types.hpp"
#include <memory>

namespace vk3d::arena {
using Instance = arena_scene::Instance;
using Box = arena_scene::Box;
Instance instance(const Box& box);
// std140: camera.xy = shake, camera.z = shadow enable, camera.w = surface transform.
using FrameData = arena_scene::FrameData;
FrameData frameData(std::uint32_t width,std::uint32_t height,float shake,float phase);
struct GroundPoint {float x,z;};
GroundPoint screenToGround(float normalizedX,float normalizedY,float aspect);

// Images/samplers returned by TextureStore are borrowed until store destruction.
struct SampledImage {VkImageView view{};VkSampler sampler{};};
enum class TextureFamily {automatic,astc,etc2,rgba8};
struct Mip {std::uint32_t width,height;Bytes payload;};
struct TextureAsset {
    std::uint32_t width=0,height=0;
    bool srgb=false; // APK floor uses linear data. Color assets may opt into sRGB.
    TextureFamily preferred=TextureFamily::automatic;
    std::vector<Mip> astc4x4,etc2rgb,rgba8;
    // ETC1 RGB blocks are legal ETC2 RGB blocks. Supply them as etc2rgb;
    // Vulkan has no separate ETC1 VkFormat and still requires enabled ETC2.
};
struct TextureDecision {TextureFamily family;VkFormat format;};
TextureDecision chooseFamily(const TextureAsset& asset,bool astc,bool etc2,bool rgba);
void validateMips(const std::vector<Mip>& mips,TextureFamily family,
                  std::uint32_t width,std::uint32_t height);
// Capability-controlled owned images; all uploads record Vulkan 1.0 barriers.
// Constructor feature flags must match features actually ENABLED on the device.
class TextureStore final {
public:
    TextureStore(VkPhysicalDevice physical,VkDevice device,bool enabledAstc,bool enabledEtc2,
                 bool enabledAnisotropy=false,float requestedAnisotropy=16);
    ~TextureStore(); // GPU must be idle / all referencing fences completed.
    TextureStore(const TextureStore&)=delete;TextureStore& operator=(const TextureStore&)=delete;
    std::uint32_t upload(VkCommandBuffer recording,const TextureAsset& asset);
    SampledImage sampled(std::uint32_t id) const;
    TextureFamily family(std::uint32_t id) const;
    void releaseStaging(); // Only after all recorded upload submissions complete.
private:struct Impl;std::unique_ptr<Impl> impl;
};
struct Shaders {std::vector<std::uint32_t> vertex,fragment;};
struct PipelineConfig {
    Context context; // borrowed color+depth renderpass, subpass 0, sample count 1
    std::uint32_t frameSlots=2,maxInstances=4096,shadowSize=1024;
};
class ScenePipeline final {
public:
    ScenePipeline(const PipelineConfig& config,const Shaders& shaders);
    ~ScenePipeline(); // All GPU work must have completed.
    ScenePipeline(const ScenePipeline&)=delete;ScenePipeline& operator=(const ScenePipeline&)=delete;
    bool shadowsAvailable() const noexcept;
    // Initial upload only; caller must complete all referencing GPU work before replacing.
    void setStaticInstances(std::vector<Instance> instances);
    // GPU work completed; caller guarantees Vulkan renderpass compatibility.
    void useCompatibleRenderPass(VkRenderPass pass);
    // completedFence must be signaled and cover ALL prior work referencing this slot.
    // Command buffer: recording primary, outside a renderpass, graphics queue family.
    // Reorders caller-owned reusable instances in place (casters first).
    // Records shadow pass -> scene pass. Does not submit, acquire or present.
    void record(VkCommandBuffer command,std::uint32_t slot,VkFence completedFence,
                VkFramebuffer colorDepthTarget,VkExtent2D extent,
                const FrameData& frame,std::vector<Instance>& instances,
                SampledImage floor,std::array<float,4> clearColor);
private:struct Impl;std::unique_ptr<Impl> impl;
};
}

namespace vk3d::arena {
// DiagnosticCopy equivalent: core transfer copy of identically formatted images.
// Caller supplies valid transfer-capable images in the stated layouts, same queue
// ownership, matching format/extent, nonoverlapping memory, and usage flags.
struct CopyImage {VkImage image;VkImageLayout layout;VkPipelineStageFlags stage;VkAccessFlags access;};
void recordDiagnosticCopy(VkCommandBuffer command,CopyImage source,CopyImage destination,VkExtent2D extent,
                          VkImageLayout destinationFinal,VkPipelineStageFlags finalStage,VkAccessFlags finalAccess);
// Query pool is borrowed, TIMESTAMP type, graphics queue timestampValidBits > 0.
// Reset/begin only AFTER fence completion for these query indices.
void beginTimestamp(VkCommandBuffer command,VkQueryPool pool,std::uint32_t firstQuery);
void endTimestamp(VkCommandBuffer command,VkQueryPool pool,std::uint32_t firstQuery);
bool pollTimestamp(VkDevice device,VkQueryPool pool,std::uint32_t firstQuery,
                   std::uint32_t validBits,float timestampPeriod,double& milliseconds);
double timestampMilliseconds(std::uint64_t begin,std::uint64_t end,std::uint32_t validBits,float period);
}

namespace vk3d::arena {
class ComputeCopy final {
public:
    // Optional diagnostic sampler->RGBA8 storage copy; not part of direct scene rendering.
    // Queue family must advertise GRAPHICS+COMPUTE; storage target must be R8G8B8A8_UNORM.
    ComputeCopy(VkPhysicalDevice physical,VkDevice device,std::uint32_t queueFamily,
                std::uint32_t frameSlots,const std::vector<std::uint32_t>& spirv);
    ~ComputeCopy(); // Caller completes GPU work before destruction.
    ComputeCopy(const ComputeCopy&)=delete;ComputeCopy& operator=(const ComputeCopy&)=delete;
    // Views cover mip 0, layer 0 of supplied images; target extent must fit both.
    // Restores source layout, leaves destination SHADER_READ_ONLY_OPTIMAL.
    void record(VkCommandBuffer command,std::uint32_t slot,VkFence completedFence,
                CopyImage source,SampledImage sourceView,CopyImage destination,
                VkImageView destinationView,VkExtent2D extent);
private:struct Impl;std::unique_ptr<Impl> impl;
};
}
