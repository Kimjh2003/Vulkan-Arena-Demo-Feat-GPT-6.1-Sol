#pragma once
#include <vulkan/vulkan_core.h>
#include <algorithm>
#include <stdexcept>
namespace graphics::vk {
struct SurfaceGeometry {
    VkExtent2D logical, image;
    float clipTransform=0; // 0..3 quarter turns, +4 horizontal mirror before rotation.
};
inline SurfaceGeometry surfaceGeometry(VkExtent2D logical,VkSurfaceTransformFlagBitsKHR transform){
    SurfaceGeometry out{logical,logical,0};
    switch(transform){
    case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR:break;
    case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:out.clipTransform=1;break;
    case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:out.clipTransform=2;break;
    case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:out.clipTransform=3;break;
    case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR:out.clipTransform=4;break;
    case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR:out.clipTransform=5;break;
    case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR:out.clipTransform=6;break;
    case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR:out.clipTransform=7;break;
    default:throw std::runtime_error("Unsupported surface pre-transform");
    }
    if(int(out.clipTransform)%2)std::swap(out.image.width,out.image.height);
    return out;
}
}
