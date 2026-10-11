# ETC/EAC texture support

- `etc_gles.h/.cpp`: ETC1 RGB (extension-gated), ETC2 RGB/sRGB,
  RGB/sRGB punchthrough alpha, RGBA/sRGBA, unsigned/signed EAC R11 and RG11.
- ETC2/EAC are GLES 3.0 core formats. ETC1 requires its advertised OES extension.
- `compressed_texture.h/.cpp`: shared ASTC/ETC complete-mip validation and upload,
  exact block byte counts, maximum size checks, texture binding restoration and
  cleanup on GL upload failure. Caller owns the returned texture.
- ETC/EAC use 4x4 blocks: 8 bytes for RGB, A1 and R; 16 for RGBA and RG.
- Upload API accepts already compressed mip bytes, not PNG/JPEG or KTX containers.
  No general image encoder or automatic ASTC-to-ETC transcoder is included.
- The demo creates a small ETC1-compatible individual-mode checker pattern and
  uploads it as ETC2 RGB, falling back to ETC1 where applicable. Failed upload
  keeps the original solid-color floor. Its linear grayscale values modulate
  floor intensity; sRGB variants are supported by the upload API separately.
- 64x64 + all 7 mip levels: 2,744 compressed bytes (payload only). The previous
  floor used no texture, so this addition demonstrates compression rather than
  reducing the original renderer's memory or proving faster rendering.
- UV/material attributes extend each vertex from 36 to 48 bytes. The existing
  one-draw scene batching is retained; the texture is uploaded only at startup
  and deleted before context destruction.

## Verification (2026-10-12)

Optimized APK build succeeded for arm64-v8a and x86_64, then installed on S24.
`tests/etc_gpu_test.cpp` ran on the S24 with a GLES pbuffer context: all eleven
formats uploaded, bindings restored, malformed/missing mip chains and unsupported
capabilities rejected. This is upload validation, not exhaustive codec/color
conformance testing. The game screenshot confirmed ETC2 floor sampling and logs
confirmed ETC1/ETC2 support plus successful floor upload; no captured app GL error.

Sources: [ETC1 specification](https://registry.khronos.org/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt),
[GLES 3.0 specification](https://registry.khronos.org/OpenGL/specs/es/3.0/es_spec_3.0.pdf).
