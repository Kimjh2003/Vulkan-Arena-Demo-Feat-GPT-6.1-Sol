# LittleGarden APK reference review

Reference: user-provided `LittleGarden-Universal.apk`, package `com.little.garden`,
version `1.0-ultra`. Analysis used ZIP entries, manifest metadata and DEX strings;
the reference app was not executed. No code or artwork was copied.

## Observations

- APK size: 2,288,524 bytes. Compile/target SDK 37, minimum SDK 26.
- Compose, DataStore and Room identifiers and bundled ART profile files are present.
- UI strings include clear start/resume actions and a demo mode with separate records.
- This is a focus timer, not a GLES game. Its package size does not establish frame
  performance, input latency, exact build configuration or visual appearance.

## Applied to Arena

- Release and installable `benchmark` variants enable R8 code optimization and
  resource shrinking. JNI class/method names are preserved by keep rules.
- Benchmark inherits release settings but uses the existing debug signing key for
  local installation. It is not a performance measurement harness or production
  signing configuration.
- Native benchmark compilation uses RelWithDebInfo with `-O2 -DNDEBUG`; packaged
  native libraries have debug symbols stripped.
- Menu, weapon selection, dash, upgrade and result actions share one themed button
  factory with touch ripple, disabled color and density-aware padding.
- No Compose/Room dependency is needed for Arena's existing small framework UI.
- The reference's ART profile cannot be reused: a useful baseline profile must be
  generated from Arena's own startup and gameplay journeys.

## Build verification

`assembleDebug` and `assembleBenchmark` succeeded. Both include arm64-v8a and
x86_64. Debug APK: 3,892,998 bytes; optimized benchmark APK: 628,289 bytes.
These are packaging results, not evidence of a measured S24 latency improvement.

The optimized APK was installed and launched on the Android 16 emulator. Menu,
native gameplay rendering and the game-over panel were observed; GLES 3.0 fallback
and direct frame output were confirmed in Arena logs, with no AndroidRuntime crash.
The emulator initially encountered its recurring System UI ANR before this check.

Build: `./gradlew.bat :app:assembleBenchmark --no-daemon` using JDK 25.
Output: `app/build/outputs/apk/benchmark/app-benchmark.apk`.

Android guidance: [APK size reduction](https://developer.android.com/topic/performance/reduce-apk-size),
[JNI keep rules](https://developer.android.com/topic/performance/app-optimization/keep-rules-overview).
