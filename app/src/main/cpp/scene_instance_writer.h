#pragma once
#include "vulkan/include/arena_api.hpp"
#include <stdexcept>

// Borrowed frame-slot stream: use only between beginFrame() and endFrame(),
// on the render owner. Conversion validates each Box; no intermediate Box vector.
class SceneInstanceWriter final {
public:
    SceneInstanceWriter(std::vector<arena_scene::Instance>& output,std::size_t limit)
        : output_(output),limit_(limit) {}
    void clear() noexcept {output_.clear();}
    void push_back(const arena_scene::Box& box) {
        if(output_.size()>=limit_)throw std::length_error("Scene instance capacity exceeded");
        output_.push_back(vk3d::arena::instance(box));
    }
private:
    std::vector<arena_scene::Instance>& output_;
    std::size_t limit_;
};
