#pragma once
#include "vulkan/include/arena_api.hpp"
namespace graphics::vk {
// Owned payloads outlive all borrowed Mip byte spans during upload.
struct FloorAsset {
    std::vector<std::vector<uint8_t>> astc,etc,rgba;
    vk3d::arena::TextureAsset asset;
    explicit FloorAsset(int preference);
};
}
