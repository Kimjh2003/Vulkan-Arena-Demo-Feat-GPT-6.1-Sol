#pragma once
#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>
#include <android/native_window.h>
#include <jni.h>
#include <vector>

namespace graphics::vk {
void check(VkResult result,const char* operation);
struct Context final {
    VkInstance instance{};
    VkSurfaceKHR surface{};
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkQueue queue{};
    uint32_t family=0;
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceFeatures enabled{};
    VkDebugUtilsMessengerEXT debug{};
    uint32_t timestampBits=0;
    Context(ANativeWindow* window);
    ~Context();
    Context(const Context&)=delete;
    Context& operator=(const Context&)=delete;
private:
    void release() noexcept;
};
}
