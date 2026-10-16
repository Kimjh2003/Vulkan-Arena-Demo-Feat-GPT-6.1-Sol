#pragma once
// World axes: X is left/right, Y is height, Z is depth. The arena is 8x8 units.
#include "vulkan/include/arena_scene_types.hpp"
using Box = arena_scene::Box;

struct GroundPoint { float x, z; };
