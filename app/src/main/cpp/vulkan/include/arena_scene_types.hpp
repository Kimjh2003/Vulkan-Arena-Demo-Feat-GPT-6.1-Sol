#pragma once
#include <cstddef>
#include <limits>
#include <type_traits>

// CPU/GPU scene ABI, shared by gameplay adapters and Vulkan. No Vulkan handles.
namespace arena_scene {
struct Box {
    float x,y,z,w,h,d,r,g,b,yaw=0;
    bool textured=false;
    float emissive=0;
};
struct Instance {float origin[3],extent[3],color[3],rotation[2],material[2];};
struct alignas(16) FrameData {float projection[4];float camera[4];};
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559);
static_assert(std::is_standard_layout_v<Instance> && std::is_trivially_copyable_v<Instance>);
static_assert(sizeof(Instance)==52 && offsetof(Instance,origin)==0 && offsetof(Instance,extent)==12 &&
    offsetof(Instance,color)==24 && offsetof(Instance,rotation)==36 && offsetof(Instance,material)==44);
static_assert(sizeof(FrameData)==32 && alignof(FrameData)==16 && offsetof(FrameData,camera)==16);
}
