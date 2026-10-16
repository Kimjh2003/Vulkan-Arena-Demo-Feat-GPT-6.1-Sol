#pragma once
#include <vulkan/vulkan_core.h>
#include <array>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <stdexcept>
namespace vk3d {
struct Bytes {const std::uint8_t* data=nullptr;std::size_t size=0;};
struct AstcImage {std::uint32_t width,height;VkFormat format;Bytes blocks;};
AstcImage parseAstc(Bytes file,bool srgb);
std::uint32_t negotiateVersion(std::uint32_t loader,std::uint32_t instance,std::uint32_t gpu);
class VulkanError:public std::runtime_error {
public:VkResult result;VulkanError(VkResult value,const char* operation);
};
struct Context {
    VkPhysicalDevice physical;VkDevice device;VkRenderPass renderPass;bool enabledAstcLdr=false;
    // Optional borrowed graphics queue for initial static mesh upload.
    VkQueue uploadQueue{};std::uint32_t uploadQueueFamily=0;
};
}
