#pragma once
// World axes: X is left/right, Y is height, Z is depth. The arena is 8x8 units.
struct Box {
    float x, y, z;
    float w, h, d;
    float r, g, b;
    float yaw = 0; // Rotation around the vertical world axis, in radians.
    bool textured = false;
    float emissive = 0;
};

struct GroundPoint { float x, z; };
