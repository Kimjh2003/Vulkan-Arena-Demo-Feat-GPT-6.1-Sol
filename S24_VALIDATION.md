# S24 smoke check — 2026-10-12

- Connected SM-S921N, Android 17 / API 37; installed package dev.demo.arena 0.1.
- Started installed app, entered gameplay, observed 3D arena and game-over panel,
  restarted and injected a short movement swipe; returned with Android Back.
- Arena runtime log: Samsung Xclipse 940 via ANGLE on Vulkan 1.3.304,
  OpenGL ES 3.2 driver 25.6.4. Direct frame output enabled.
- No AndroidRuntime exception in captured app PID logs.
- SurfaceFlinger timestats: game Surface layers reported 2,437 and 866 frames,
  zero dropped frames, and predominantly 8ms presentation intervals.
  Reported averages around 125 FPS reflect the system histogram calculation;
  do not interpret them as a precise physical panel refresh measurement.
- Short sample includes terminal game-over rendering and restart, not sustained
  high-enemy combat. Input-to-display latency, thermal stability and all GLES 3.2
  optional features were not measured.

Local captures: build/s24-runtime.txt, build/s24-timestats.txt, build/arena-s24.png.
