# Advanced GLES demo pipeline — 2026-10-12

## Implemented

- GPU instancing: static 36-vertex cube mesh (864 bytes) and 52-byte instance
  records for origin, dimensions, color, rotation and material. This replaces
  per-box CPU mesh expansion (36 x 48 = 1,728 bytes per box per frame). GPU still
  processes cube vertices; this reduces CPU work and upload volume, not geometry.
- Frame UBO: 32-byte std140 projection/camera block shared by scene and shadow
  passes. Updated once per frame and bound at binding point 0.
- Dynamic directional shadow map: 1024x1024 depth24 texture, depth-only FBO,
  polygon/depth bias and nine PCF sample positions using hardware depth comparison.
  Taller non-emissive instances cast shadows; thin grid lines/particles do not.
  FBO completeness failure disables shadows while retaining scene rendering.
- Fragment lighting: ambient/diffuse, Blinn-Phong specular, rim highlight and
  distance fog. Bullets/particles gain emissive material shading (no bloom pass).
- EXT_texture_filter_anisotropic: exact extension check and runtime maximum,
  capped at 8x for the ETC floor; normal trilinear sampling when absent.
- Existing window MSAA, ETC2 floor, driver diagnostics and sparse GPU timer remain.

## Module ownership

`ScenePipeline` owns mesh/instance/UBO buffers, shader and shadow texture/FBO.
`GlesRenderer` owns EGL/window, floor texture, optional extensions and frame output.
`SceneBuilder` provides scene primitives/materials without GL dependencies.
All GL resource creation/draw/cleanup stays on the renderer thread.

Typical frame: clear main target → upload instances/UBO → depth shadow draw →
restore main framebuffer/viewport → shaded instanced draw → EGL swap. The main
color target remains direct; the shadow pass introduces its own offscreen depth
target. There is no fullscreen color-copy or bloom pipeline in normal mode.

## Compatibility and verification

These scene features use GLES 3.0 core APIs plus optional anisotropy. Existing
3.2 → 3.1 → 3.0 EGL negotiation and 320/310/300 shader selection remain.
They do not exercise every GLES 3.2 feature or establish a hardware limit.

- Benchmark APK built for arm64-v8a/x86_64 and installed on S24 Android 17.
- S24 GLES 3.2 logs confirmed instancing/UBO, 1024 shadow map and 8x anisotropy.
- Screenshot confirmed dynamic shadows and shaded ETC floor in gameplay.
- `tests/scene_gpu_test.cpp` ran on S24: GLSL 300 and 320 render nonblack scene
  pixels, support viewport-size changes and clean up without GL errors.
  This tests the GLSL 300 path on a 3.2 driver, not a physical GLES 3.0-only GPU.
- Captured app logs contained no ArenaGL/initialization/frame error.
- Early sparse GPU samples ~5–7ms versus ~1.5ms in earlier simpler scene samples.
  Different runs/scenes/thermal states: not a controlled performance comparison.
  PCF, additional shadow draw and anisotropy deliberately increase GPU workload.
  No sustained frame-rate or input-latency claim is made.

Captures: build/s24-pipeline.png and build/s24-pipeline-log.txt.

References: [GLES 3.0 specification](https://registry.khronos.org/OpenGL/specs/es/3.0/es_spec_3.0.pdf),
[anisotropy specification](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_filter_anisotropic.txt).
