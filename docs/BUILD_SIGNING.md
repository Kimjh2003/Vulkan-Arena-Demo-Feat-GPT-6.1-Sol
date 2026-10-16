# Manifest, signing and dependency policy

- Android 9–17 (API 28–37), Vulkan 1.0 required; launcher Activity is intentionally exported.
- App category is `game`; cleartext traffic is disabled. No network or storage permissions are requested.
- `allowBackup=false` disables Android cloud backup. OEM device-to-device transfer behavior can differ.
- Debug, benchmark and profile retain the existing local debug keystore, so local APK updates keep their signing identity.
- Signing enables v2/v3 and disables v1 (minSdk 28). The verified APK uses v3 on the supported Android range. This does not change GPU behavior.
- Benchmark is optimized and non-debuggable. Profile is deliberately debuggable for Sokatoa capture.

## Production signing

Set all four environment variables in your build environment:

| Variable | Value |
| --- | --- |
| `ARENA_RELEASE_STORE_FILE` | Path to an existing production keystore |
| `ARENA_RELEASE_STORE_PASSWORD` | Keystore password |
| `ARENA_RELEASE_KEY_ALIAS` | Signing key alias |
| `ARENA_RELEASE_KEY_PASSWORD` | Signing key password |

Run `./gradlew.bat :app:assembleRelease`. Without these variables, release remains unsigned;
use `:app:assembleBenchmark` for a locally installable APK. Partial configuration fails explicitly.
No production key is generated or stored in this project. Keep the signing key for future updates;
a production certificate cannot update an already installed debug-signed app directly.

## Dependency locks

`app/gradle.lockfile` records the resolved compile/runtime module versions. Commit it with build changes.
After an intentional dependency/toolchain update, regenerate using:

```powershell
./gradlew.bat :app:dependencies --write-locks --no-daemon
./gradlew.bat :app:assembleDebug :app:verifyBenchmarkOptimization :app:assembleProfile :app:assembleRelease --no-daemon --max-workers=2
```

Locks cover app classpaths, not Gradle plugin artifacts, SDK or NDK downloads. Existing explicit
AGP/NDK/CMake versions remain in the build scripts. Runtime modules remain Kotlin stdlib and annotations;
R8 removes unused code. Locking is for reproducibility, not a runtime performance improvement.

References: [Android signing](https://developer.android.com/studio/publish/app-signing),
[Gradle dependency locking](https://docs.gradle.org/current/userguide/dependency_locking.html).

## Verification — 2026-10-16

- Debug, benchmark, profile and unsigned release builds passed for both configured ABIs.
- `verifyBenchmarkOptimization` passed again without `--write-locks`.
- All four merged manifests matched SDK, backup, cleartext, permissions and variant debug policies.
- Benchmark APK: 435,353 bytes; v3 signature verification and 16 KB ZIP alignment passed.
- Its signing certificate matches `VulkanArena-0.2-r8-dependencies.apk`.
- Installable artifact: `releases/VulkanArena-0.2-manifest-signing.apk`.
- No ADB device was connected during this check; on-device install/runtime verification was not repeated.
