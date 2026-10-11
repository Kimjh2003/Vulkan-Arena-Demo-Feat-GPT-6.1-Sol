# GLES demo additions — 2026-10-12

## Visible feature: window MSAA

EGL config selection prefers at least 4 samples, then 2, then a non-multisampled
config. Actual GL sample count is logged. Window MSAA reduces triangle-edge
aliasing using the existing default framebuffer; no extra application-owned
offscreen textures or fullscreen copy pass are added. The driver still resolves
multisampling and additional GPU memory/work can be required. This is EGL/GLES
multisampling, not an optional GL extension and not a latency optimization.

## Optional extension module

`gl_extensions.h/.cpp` uses exact extension-name matching via `glGetStringi` and
loads extension entry points with `eglGetProcAddress` only when supported.

- `GL_KHR_debug`: driver error/high/medium diagnostic callback to ArenaGL logs.
  Notifications are suppressed; synchronous debug output is not enabled.
- `GL_EXT_disjoint_timer_query`: one elapsed-time query every 120 rendered frames.
  Results are polled without waiting. Busy queries skip new samples, and disjoint
  intervals are discarded. Counter-bit and function availability are checked.
  Timing covers scene GL commands, excluding swap/presentation, CPU scene building
  and Android UI; it is not input latency or a full-frame benchmark.
- Absent extensions or entry points leave normal rendering enabled.
- Query/callback cleanup occurs before the EGL context is released.

## Validation

Optimized benchmark APK compiled for arm64-v8a and x86_64 and was installed on
the connected S24 SM-S921N, Android 17. Actual context GLES 3.2:

- Window MSAA: 4 samples.
- KHR_debug and EXT_disjoint_timer_query both advertised and enabled.
- Native gameplay screenshot confirmed arena, HUD and controls rendering.
- Initial sparse GPU samples approximately 1.0–1.6ms; not a sustained performance
  result or comparison against the previous APK.
- Captured app logs contained no initialization/frame error or ArenaGL warning.

Fallback paths are capability-gated in source but were not run on a device lacking
these extensions during this change.

Specifications: [KHR_debug](https://registry.khronos.org/OpenGL/extensions/KHR/KHR_debug.txt),
[EXT_disjoint_timer_query](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_disjoint_timer_query.txt).
