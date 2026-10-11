# Shared modules and codec-independent optimization

## Ownership

| Module | Responsibility |
|---|---|
| GlCapabilities | One context-local snapshot of extensions, codec formats, compute limits and anisotropy |
| GlExtensions | Optional debug/timer entry points; consumes snapshot rather than enumerating repeatedly |
| compressed_texture | Shared ASTC/ETC mip validation, upload, filtering, binding restoration, error cleanup |
| material_texture | Same grayscale material payload in ETC1/ETC2/ASTC, preference selection and fallback |
| ScenePipeline | Codec-independent instances, frame UBO, shadow map and lighting |
| ArenaTheme / ArenaDialog | Shared buttons, panels, option rows and reusable radio choice lists |
| GraphicsSettings / GraphicsPicker | Persisted texture preference and presentation; no GL calls on UI thread |

The current renderer takes one capabilities snapshot per EGL context. Compatibility
query wrappers remain for standalone tools/diagnostic paths. Each codec adapter
receives the same sampling policy: complete mips, trilinear minification, linear
magnification, repeat wrapping, explicit last level and up to 8x supported
anisotropy. ASTC decode-mode options remain specific to ASTC where applicable.

## Common effects and selection

Graphics settings are accessible through the Graphics action in control settings.
Auto/ETC1/ETC2/ASTC preferences apply to the next round. Unsupported formats fall
back to supported ASTC -> ETC2 -> ETC1 -> white. Existing selection UI and graphics
choices share ArenaDialog. Graphics choices save immediately; control changes
still require Save.

All selected floor textures use the same linear-color shader, instanced geometry,
PCF shadow, lighting/fog and MSAA output. ASTC floor uses LDR 4x4 constant blocks;
its decoded intensities match the ETC pattern. This is a demo pattern generator,
not a general-purpose encoder or KTX loader. Codec support, alpha/color semantics,
compression ratio and physical performance remain format-dependent.

## Optimization layers

- C++: ThinLTO confirmed in NDK compile commands for optimized variants, hidden
  non-JNI symbols, function/data sections and linker dead-section removal.
  Native JNI exports retain visibility via JNIEXPORT. O2 remains enabled.
- SceneBuilder: caller-owned cached vector reused each frame rather than creating
  a fresh scene vector; ScenePipeline already retains its instance vector/buffer.
- GLES: instancing and common material binding apply to every codec. Optional
  timer disjoint polling occurs only for pending/sampling intervals.
- R8: AGP 9.3 optimization DSL enables code/resource optimization for release and
  benchmark. JNI keep rules live in src/main/keepRules/jni.keep. R8 optimizes the
  Kotlin/Java UI/JNI callers; it does not optimize C++ or compress texture bytes.

## Validation — S24 Android 17

`tests/texture_family_gpu_test.cpp` ran against actual GLES: ETC1, ETC2 and ASTC
each selected the requested format, decoded the expected grayscale (223 ±2),
had matching filter/anisotropy state and rendered through the shared shadow
pipeline without GL error. Synthetic unsupported capabilities reached white
fallback. This does not prove identical frame time or full codec conformance.

Benchmark build and Android-test APK compilation succeeded after R8/API changes.
Final optimized APK: 637,289 bytes, arm64-v8a and x86_64 included. Installed and
run on S24: automatic selection used ASTC 4x4 with 8x shared sampling, and normal
gameplay/shadows/HUD were observed without captured JNI/GL errors. The controls
dialog's Graphics/Cancel/Save actions were visible. Automated inspection of the
nested picker was interrupted by active gameplay; its selection callbacks are
compiled but each picker choice was not exercised through Android UI automation.
Codec rendering itself was tested for all three families in the GPU test above.

Reference: [Android R8 optimization](https://developer.android.com/topic/performance/app-optimization/enable-app-optimization).
