#pragma once
#include "arena_api.hpp"
#include <algorithm>
#include <cmath>
namespace vk3d::arena::detail {
inline void validateInstances(const std::vector<Instance>& instances){
    for(const auto& i:instances){
        for(float x:i.origin)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite instance origin");
        for(float x:i.extent)if(!std::isfinite(x)||x<=0)throw std::invalid_argument("Invalid instance extent");
        for(float x:i.color)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite instance tint");
        for(float x:i.rotation)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite instance rotation");
        for(float x:i.material)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite instance material");
        if(std::abs(i.rotation[0]*i.rotation[0]+i.rotation[1]*i.rotation[1]-1)>1e-3f)
            throw std::invalid_argument("Rotation must be cosine/sine unit pair");
    }
}
inline std::uint32_t partitionCasters(std::vector<Instance>& instances){
    const auto end=std::partition(instances.begin(),instances.end(),[](const Instance& i){
        return i.extent[1]>.08f&&i.origin[1]>=0&&i.material[1]<.1f;
    });
    return static_cast<std::uint32_t>(end-instances.begin());
}
}
