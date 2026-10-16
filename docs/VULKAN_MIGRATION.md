# Vulkan renderer migration

The Android game now calls Vulkan directly. The legacy GLES implementation stays
as reference source, but the app's native target does not compile or link EGL/GLES.
Gameplay, Kotlin UI, controls, records and audio share the existing modules.

## Modules

- `game_runtime`: owns one gameplay session, reusable scene staging and renderer;
  commits initialization only after uploads succeed. Diagnostics alone are atomic.
- `native.cpp`: JNI adapter, direct-buffer checks, RAII Android window reference
  and shared exception boundary; no simulation/render orchestration lives here.
- `arena_instances.hpp`: shared finite/extent/rotation validation and shadow-caster
  partitioning for static and dynamic instance streams.

- `vulkan_context`: Android surface, physical/logical device, one graphics+present
  queue, enabled compression/anisotropy features and optional debug validation.
- `vulkan_swapchain`: FIFO presentation, compatible color/depth renderpass,
  per-image depth/framebuffer/present semaphore, presentation resource ownership.
- `FrameClock.kt`: coalesced Android display pulses and lifecycle cancellation.
- `DisplayProfile.kt`: receives the current window's display identity, mode,
  refresh rate, rotation, dimensions, wide-color capability and state; refreshes
  through DisplayManager callbacks and unregisters with the render surface.
  These are diagnostic metadata, not overrides of Vulkan surface capabilities.
- `vulkan_renderer`: two frame slots, input preparation, command recording,
  submission, image ownership tracking, output recreation and sampled GPU timings.
- `vulkan_floor`: identical procedural material in ASTC 4x4, ETC RGB and RGBA8.
- `vulkan/include` and `vulkan/src`: adapted user-supplied rendering API, reusable
  scene pipeline, texture upload, policies and optional diagnostic copies.

## CPU/GPU resources

Each frame slot reserves 4096 instances once. Static floor/grid/cover boxes are
converted and uploaded once; JNI converts only dynamic SceneBuilder boxes into
that slot's instance vector. ScenePipeline
partitions the same vector in place to put shadow casters first; it no longer
creates a copy. A slot is only reused after its submit fence completes.

Each slot uses one host-visible buffer/allocation for its UBO and dynamic
instances, rather than two allocations. UBO offset is zero; the vertex stream
starts at an offset aligned to max(16, minUniformBufferOffsetAlignment).
The buffer stays mapped until destruction. Coherent memory
skips explicit flush; noncoherent memory flushes the changed range rounded to nonCoherentAtomSize
at both ends of each written subrange (or the allocation end). The static cube and
static instance stream upload once through
a temporary staging buffer into a buffer preferring device-local memory.
Descriptors update only when the floor view/sampler changes. Device limits used
by normal scene recording are cached at pipeline construction.

## Android frame flow

Display configuration comes from Android, not a device preset. Choreographer
provides the actual frame pulses, so a reported refresh rate is not converted
into a fixed sleep interval. Vulkan queries the surface's supported formats,
extent and current transform when building the swapchain. Java display rotation
is not applied again on top of Vulkan's transform. Wide-color capability alone
does not enable HDR or change the current SDR swapchain. ArenaDisplay and
ArenaVulkan logs allow the two system-provided views to be compared.

`prepare()` waits for the slot and acquires an image **before** Kotlin samples
touch input and consumes dash/upgrade requests. It returns integer 1 for timeouts or
an out-of-date swapchain. The game then updates and submits one frame.

Acquire semaphores and command buffers belong to frame slots. Render-finished
semaphores belong to swapchain images, not frame slots. Acquiring that image
again makes its previous presentation semaphore reusable. Submit fences alone
do not prove presentation completion. Each target also remembers the fence of
the last rendering submission using its depth attachment.

Fences are reset after a complete command buffer is ready, immediately before
submission. No device/queue idle is used in the normal frame loop. Initialization,
recreation and teardown wait for completion. An out-of-date/suboptimal result
recreates the swapchain on the next frame. Compatible color/depth formats retain
the pipelines, static buffers, descriptors and shadow images; a format change
rebuilds the pipeline. Surface-size callbacks refresh metadata while the same
worker observes native window size changes, preserving the live simulation.
Activity pause and surface destruction stop the worker, await its semaphore
teardown acknowledgement and release native resources; resume
uses the existing game's round restart behavior.

FrameClock uses Android Choreographer pulses on the UI thread and a semaphore
with at most one pending pulse for the render worker. Busy frames coalesce old
pulses instead of queuing stale input/render work. Pause cancels the callback and
wakes the worker before awaiting teardown. Presentation calls vkQueuePresentKHR directly.
This is display-vsync pacing plus FIFO, not an adaptive presentation timing SDK.

Swappy 2.1.3 was initially integrated, but synchronization validation found its
internal fence injection reusing presentation semaphores for different images.
The final app removes that dependency and uses its own image-indexed semaphores.
C++ uses the static STL again, avoiding the shared runtime packaging overhead.
The swapchain matches currentTransform. Its image extent is the natural
orientation extent, swapping width/height for 90/270 degrees. FrameData.camera.w
selects clip-space pre-rotation (and mirror, if needed), while projection uses
logical window dimensions. Shadows are independent of display pre-rotation.

## Application memory and thread ownership

The NDK arm64-v8a baseline is used, without SVE, LSE or a specific CPU model
requirement. ISA width is not a RAM budget. Instance GPU stride remains 52 bytes
instead of padding every instance to 64; std140 FrameData and CPU FrameState
storage retain 16-byte alignment. Vulkan allocation/binding and flush alignment
come from device requirements, not an assumed page or cache-line size.

The render worker allocates one 56-byte native-order direct JNI buffer per round.
Native copies 14 32-bit words into it, preserving dash's float bits. Frame and
prepare return integer status codes; no JSON/string allocation or parsing occurs
on this path. UI receives immutable HUD snapshots only at publication time, and
never reads the worker's reusable buffer asynchronously. Errors still log native
details; initialization can return a one-time error string.

One application process and one ArenaRenderer worker own gameplay, command pool,
queue and mapped writes. Choreographer signals a semaphore with at most one
pending permit, coalescing old pulses. A separate one-shot semaphore acknowledges
native teardown before another worker may start. UI diagnostics use lock-free
64-bit atomics; the renderer no longer takes a shared mutex every frame. GPU
semaphores/fences remain separate from Java synchronization. Teardown still waits
for GPU completion; this does not guarantee a bound on a stalled driver.

Simulation vectors reserve their ordinary working capacity at round start
(120 enemies/shots, 128 player bullets, 240 hostile bullets, 160 particles).
Existing gameplay limits are unchanged; player bullet reserve is not a hard cap.
The render path rejects a total above 4096 instances instead of overrunning GPU
storage. Two GPU dynamic streams use 425,984 bytes plus small aligned UBO regions;
CPU instance staging has the same reserved payload size. This excludes scene
boxes, textures, attachments, allocator overhead and driver memory. Static/dynamic
separation adds up to one draw per pass in exchange for fewer per-frame uploads;
actual performance must be measured on the device.

## Capabilities and differences from GLES

- Vulkan 1.0 core renderer, Android API 28 minimum; no Vulkan 1.4-only features.
- ASTC 4x4 / ETC2 RGB / RGBA8 fallback always has an actual payload.
- Existing ETC1 option means ETC1-compatible blocks through ETC2 RGB; Vulkan has
  no separate ETC1 format. If ETC2 is unavailable, a different family is used.
- 1024-square sampled depth shadows, manual nearest 3x3 PCF, optional anisotropy.
- One sample per attachment; the previous GLES window MSAA is not reproduced.
- Diagnostic ComputeCopy and transfer helpers remain optional, outside gameplay.
- Timestamp samples are GPU scene duration, not touch-to-display latency.
- Vulkan is required by the manifest; there is no automatic GLES fallback.

## C++ and R8 build contract

R8 applies to Kotlin/Java, not native machine code. AGP's release optimization
block enables code/resource shrinking and supplies the default JNI downcall keep
rule. The redundant local native-name rule was removed; no package-wide keep or
disable-optimization rule is added. Check the generated configuration/mapping
under `app/build/outputs/mapping/benchmark` when adding JNI or reflection.

All native implementation targets share hidden and hidden-inline visibility,
function/data sections and ThinLTO for optimized configurations. The final shared
library also uses section garbage collection and `arena.exports.map`: only four runtime
JNI entry points are public in optimized builds; Debug adds two diagnostic calls. Undefined export names fail the link, and the map is
a link dependency. Static C++ runtime implementation symbols are private; native
exceptions remain internal to libarena.so. Baseline arm64-v8a and 16 KB page
alignment are unchanged. GPU formats, shaders and game rules are unchanged.

## Verification

Verified on the Android 16 / API 36.1 x86_64 emulator (SwiftShader):
- Debug, instrumentation and R8/ThinLTO benchmark builds succeeded for both ABIs.
- CPU API and gameplay regression checks passed.
- Offscreen GPU checks passed for ASTC, ETC2 and RGBA8 with shadows on/off.
- Static/dynamic separation produced byte-identical readback pixels to the
  combined stream in all six offscreen cases; compatible renderpass replacement
  and combined UBO/vertex storage were exercised by the same tests.
- Android integration passed controls, settings, HUD, submitted Vulkan frames,
  resize/restore, background/resume, gameplay Back and home exit.
- Resize/restore retained the same render worker. The binary HUD test verified
  optional fields and exact float transport. ARM64 ELF LOAD alignment and APK
  zip alignment satisfy the 16 KB page checks; APK signature verification passed.
- The final renderer's synchronization-validation run reported no Vulkan
  validation errors or warnings during rendering and surface recreation.
- The optimized universal APK ran gameplay; the initial Vulkan build measured 708,733 bytes
  (before the display metadata receiver was added);
  its native dependencies include Vulkan and exclude EGL/GLES.

The screenshot in the README is from that optimized emulator build. Software
GPU timings are not representative of S24 performance.

Build optimized and debug APKs with `:app:assembleBenchmark :app:assembleDebug`.
`tests/vulkan_api_test.cpp` covers CPU policies; `tests/vulkan_gpu_test.cpp` covers
offscreen rendering, compressed/fallback textures, shadows and reusable slots.
`tests/game_test.cpp` remains the gameplay regression test.

Use `tools/compile_vulkan_shaders.py --glslc <NDK glslc>` to regenerate GLSL/SPIR-V
and the embedded C++ header. Validate binaries with `spirv-val --target-env vulkan1.0`.
The APK uses embedded binaries and does not compile shaders at runtime.

After the memory/thread changes, the optimized universal APK measured 659,765
bytes and ran through gameplay and the game-over result screen on the emulator.
The full Android integration run with synchronization validation passed with
zero validation warning/error messages.

After C++ runtime extraction, shared validation and restricted ELF exports,
Debug/Benchmark and instrumentation builds passed for both ABIs. CPU validation
checks and all six GPU pixel-comparison cases passed again. Android integration
passed with zero Vulkan validation messages, and the R8 APK started gameplay.
Both libraries expose exactly six JNI functions. The universal APK decreased
from 659,765 to 435,693 bytes; arm64 libarena.so from 272,624 to 158,264 bytes,
x86_64 from 276,264 to 166,880 bytes. DEX remained 50,188 bytes: this round's size
reduction comes from native linking, not additional R8 code removal.

Physical-device performance and thermal behavior require a new S24 run. Earlier
GLES measurements do not establish Vulkan performance.

## Kotlin ownership and publication modules

These are responsibility modules inside the existing app build, with no added
runtime dependency or separate Gradle artifact:

| Module | Responsibility |
|---|---|
| MainActivity | Navigation and Android lifecycle |
| GameRoundView | Round UI, dash/upgrade/result presentation and one-time record |
| WindowPresentation | Shared safe insets and immersive system bars |
| NativeBridge | Six native downcalls, independent of Activity |
| RenderSession | One render owner, frame clock and teardown acknowledgement |
| RoundActions | Atomic one-shot dash and upgrade requests |
| NativeStateBuffer / HudState | Reused 56-byte buffer and immutable changed-state snapshots |
| StateMailbox | Latest-state delivery through one reused main-thread Runnable |
| TouchInput | UI-owned pointer tracking and immutable input publication |
| UiText | Shared time and cooldown formatting without Formatter |

Touch normalization uses a UI-owned four-float scratch array, removing Pair and
boxed Float temporaries. Identical input reuses the previous immutable snapshot;
reset reuses a zero snapshot. The render thread never reads the scratch array.
HUD publication compares the 13 state words and excludes the sound-event word.
The UI mailbox coalesces pending snapshots; closing it suppresses old callbacks.
Native audio events remain consumed per frame. HUD labels change only when their
visible fields change; dash labels are rebuilt only when displayed tenths change.
JNI still carries primitive input values and one direct output buffer, without
creating a per-frame call object. Native input is sampled after frame preparation.

JNI exports now use NativeBridge, with AGP's existing native-name keep rule.
The teardown semaphore still waits for native cleanup before a new session;
this refactor does not make a stalled driver shutdown asynchronous.

Kotlin refactor verification: Debug, instrumentation and R8 Benchmark builds
passed for both ABIs. Emulator integration exercised duplicate input identity,
exact wire floats, event-only publication suppression, latest-state mailbox
coalescing and close cancellation, alongside controls/HUD/results/resize/resume/
Back navigation. Vulkan validation loaded and reported zero warning/error messages.
Both optimized libraries still export exactly six NativeBridge JNI symbols.
APK signing and 16 KB APK/arm64 ELF alignment checks passed.
The universal APK remains 435,693 bytes (425.5 KiB); DEX increased from 50,188 to
52,804 bytes while alignment absorbed the change. This is allocation/publication
work reduction, not an APK size reduction or a measured physical-device speedup.

S24 physical integration and Sokatoa capture/replay results are recorded in
[S24 Sokatoa validation](S24_SOKATOA_VALIDATION.md). The profile build permits
GFXR injection while retaining native NDEBUG/ThinLTO and avoiding validation-only
instance extensions in captured calls.

## Shared scene contract and direct frame-slot writing

`vulkan/include/arena_scene_types.hpp` is the common CPU/GPU data contract:
Box, 52-byte Instance and 32-byte std140 FrameData. IEEE 32-bit float, structure
layout and every instance attribute offset are compile-time assertions. Vulkan
attribute declarations use offsetof instead of duplicate numeric instance offsets.
The gameplay Box and Vulkan Box are aliases of this one definition.

SceneBuilder now accepts an output sink. GameRuntime requests a borrowed
SceneInstanceWriter once after successful frame acquisition and writes dynamic
instances directly into that reusable slot vector. The prior 4096-Box intermediate
reservation (about 192 KiB on these ABIs), Box vector writes, extra conversion Box
and per-object renderer dispatch are removed. Each Box still passes conversion
validation and the pipeline still validates input and slot fence completion.
The writer is render-thread-only and must not outlive its active frame. Its limit
is checked before each append. Native pipeline limits also include static objects.

The existing renderer continues owning static geometry, textures, pipelines,
frame-slot storage and synchronization. Descriptor write structures are now
constructed only when image/sampler bindings change. This does not claim fewer
Vulkan descriptor updates: updates were already cached; it removes repeated CPU
preparation. Frame fences, image semaphores, barriers and FP32 shaders are unchanged.
Kotlin continues calling the same NativeBridge and reused 56-byte state buffer;
no per-object JNI call or new runtime dependency is introduced.

Verification on the Android x86_64 emulator:
- Debug/instrumentation/R8 Benchmark APK builds for arm64-v8a and x86_64 passed.
- Gameplay regression passed. New scene_stream_test compares direct versus staged
  instances byte-for-byte across both control modes for 600 updates each, verifies
  stable vector storage, rotated/emissive objects and capacity/invalid-box rejection.
- Six GPU cases (ASTC/ETC2/RGBA8 x shadow off/on) retain identical readback pixels
  between staged and direct writing, including alternating slots and compatible
  renderpass reuse.
- APK signature and 16 KB zip alignment checks passed.
- Universal APK: 435,693 -> 435,677 bytes; DEX stays 52,804 bytes. ARM64 native:
  158,264 -> 158,040 bytes; x86_64: 166,880 -> 166,864 bytes.

This establishes equivalent output and removed staging storage, not a measured
S24 frame-time or touch-latency improvement. The S24 was not connected for this run.

Android integration also passed controls, settings, HUD, submitted Vulkan frames,
resize/restore with the same render owner, pause/resume, gameplay Back and home
exit. Initial runs were obstructed by emulator Bluetooth crash and System UI ANR
dialogs; after clearing those environment failures, the unchanged integration
suite passed. These emulator failures were not counted as successful app tests.

## R8 and dependency boundary cleanup

Current toolchain: AGP 9.4.1 / Gradle 9.6 / built-in Kotlin. No compiler, SDK or
library version was changed by this cleanup. Runtime dependency inventory is
kotlin-stdlib 2.2.10 and JetBrains annotations 13.0. R8 was already eliminating
unused Kotlin members; no artificial stdlib exclusion, broad keep, ignored missing
class rule or removed null checking is used to manufacture a size gain.

- Main NativeBridge has four downcalls: start, prepare, frame, stop.
- NativeDiagnostics exists only in src/debug. Instrumentation invokes it directly;
  Activity reflection wrappers are removed. No new reflection keep rule is needed.
- CMake selects a six-symbol Debug export map (four runtime, two diagnostic) and
  four-symbol optimized map. Undefined exports fail linking. Both ABI libraries
  were inspected with llvm-nm: the expected symbols and counts match all three APKs.
- Native diagnostic atomic counters and output-generation checks are excluded
  under NDEBUG, removing diagnostic work from optimized frame execution.
- Gameplay dependencies terminate at GameRuntime. Graphics exposes the neutral
  scene contract/API and privately links Android/log; it does not expose gameplay.
- ComputeCopy is a separate EXCLUDE_FROM_ALL static target vulkan_arena_compute.
  Consumers explicitly link that target when needed. The app does not link it;
  its core archive has five objects and no arena_compute.cpp object. Previously
  unused compute code was already removed by the linker, so this is build graph
  reduction rather than a claim of additional compute runtime savings.

Run `./gradlew.bat :app:verifyBenchmarkOptimization` to build/audit the optimized
artifact. It checks retained JNI class identity, absence of debug diagnostics in
mapping, no global R8 disable flags and exactly the two intended native ABI files.
It records the resolved runtime dependency inventory and APK size in
app/build/reports/optimization/benchmark.json. The comments-only legacy JNI keep
file was removed; the actual AGP native downcall rule remains in effect.

Debug, instrumentation, Benchmark and Sokatoa Profile builds passed. APK/ELF
inspection confirms Debug includes NativeDiagnostics while Benchmark/Profile do
not, and native dependencies include no EGL/GLES libraries.
Universal benchmark APK: 435,677 -> 435,309 bytes (368 bytes smaller).
DEX stays 52,804 bytes; native ARM64 158,040 -> 157,576 bytes, x86_64
166,864 -> 166,496 bytes. The measured size change is native, not new R8 DEX savings.
No new S24 latency or frame-rate claim follows from these build changes.

The updated Android integration suite passed on the emulator with direct Debug
NativeDiagnostics calls: controls/settings/HUD, actual Vulkan frames, resize,
pause/resume and both Back paths. Cold SurfaceView startup now waits for actual
frames before asserting focus, retaining both assertions and the existing frame
deadline. Earlier attempts were obstructed by launcher/System UI ANR dialogs.
The optional ARM64 vulkan_arena_compute target also builds when explicitly chosen.
Optimized APK signing, zip/ELF 16 KB alignment checks passed for the packaged ABIs.

The final four-entry-point R8 APK also ran Vulkan gameplay through the game-over
result on the emulator without JNI linkage errors. S24 performance was not retested.
