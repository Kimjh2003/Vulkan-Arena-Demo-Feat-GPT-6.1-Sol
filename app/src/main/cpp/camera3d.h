#pragma once
#include <algorithm>
#include <cmath>
#include "render_types.h"
#include "gameplay/world.h"

// Fixed perspective camera aimed at the center of the ground plane.
struct Camera3D {
    static constexpr float eyeHeight = 7.f;
    static constexpr float eyeDepth = 7.f;
    static constexpr float tanHalfFov = .46630766f; // 50-degree vertical FOV
    static constexpr float diagonal = .70710678f;
    static constexpr float nearPlane = .1f;
    static constexpr float farPlane = 50.f;

    static GroundPoint screenToGround(float sx, float sy, float aspect) {
        aspect = std::max(.1f, aspect);
        float rayX = (sx*2.f-1.f)*tanHalfFov*aspect;
        float rayY = -diagonal+(1.f-sy*2.f)*tanHalfFov*diagonal;
        float rayZ = -diagonal-(1.f-sy*2.f)*tanHalfFov*diagonal;
        float distance = -eyeHeight/rayY;
        return {World::toGround(rayX*distance), World::toGround(eyeDepth+rayZ*distance)};
    }
};
