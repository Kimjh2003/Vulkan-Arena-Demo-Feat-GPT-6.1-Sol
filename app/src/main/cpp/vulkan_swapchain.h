#pragma once
#include "vulkan_context.h"
#include "vulkan_surface_transform.h"
#include "vulkan/src/arena_gpu.hpp"

namespace graphics::vk {
struct Swapchain final {
    Context& context;
    VkSwapchainKHR handle{};
    VkRenderPass pass{};
    VkExtent2D extent{};
    SurfaceGeometry geometry{};
    VkFormat colorFormat{},depthFormat{};
    struct Target {
        VkImage image{};VkImageView view{};
        vk3d::arena::gpu::Image depth{};
        VkFramebuffer framebuffer{};VkSemaphore rendered{};
        VkFence lastSubmission{};
    };
    std::vector<Target> targets;
    Swapchain(Context& context,ANativeWindow* window);
    ~Swapchain();
    Swapchain(const Swapchain&)=delete;
    Swapchain& operator=(const Swapchain&)=delete;
private:
    void release() noexcept;
};
}
