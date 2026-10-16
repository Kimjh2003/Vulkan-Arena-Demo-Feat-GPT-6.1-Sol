# Third-party UI attribution

## CopilotKit / OpenIntelligentUI

- Repository: https://github.com/CopilotKit/OpenIntelligentUI
- Reviewed revision: `f6e4388b26a64b9a0714943b08a1ce622b924eec` (2026-10-12).
- Source: `packages/design-system/src/index.ts`, `THEME_CSS` dark theme.
- Copyright (c) Atai Barkai. MIT License.
- Full upstream notice shipped in `app/src/main/assets/licenses/OpenIntelligentUI-MIT.txt`.

`ArenaTheme.kt` adapts the dark semantic colors, border opacity and 12px corner
radius to Android colors and density-independent dimensions. `GameHud.kt` and
the home screen use these tokens. The panels and gameplay binding are local
Kotlin implementations. The React/Next.js application, LLM backend, hosted
services and generative UI runtime are not included. No network service is
needed for the HUD.

The MIT permission allows modification and distribution, including commercial
use, with its copyright and permission notice retained in copies or substantial
portions. The upstream disclaimer is retained verbatim in the APK asset.

## ASTC reference

User-provided reference archive:
`Gemini-3.6-Flash-and-Kimjh-Vibe-Coding-Feat.ASTC-main.zip`.
The GLES adaptation is identified in `astc_gles.h/.cpp`; the reference's
Apache-2.0 license is preserved in source and APK assets. The full local reference
checkout is excluded from this repository.

The previous ASTC adaptation retains its Apache-2.0 notice in
`app/src/main/assets/licenses/ASTC-Apache-2.0.txt`.

## Vulkan rendering API input

The user supplied `vulkan-arena-api.zip` as the migration input. Its rendering API,
shaders and contract are adapted in `app/src/main/cpp/vulkan`. Local adaptations
include persistent mapping, in-place reusable instance batches, cached limits,
conditional descriptor updates and static vertex staging. The supplied archive
has no standalone license file; no new license is inferred for that input.

## Frame pacing evaluation

Swappy 2.1.3 was evaluated during migration. The final app uses Android's built-in
Choreographer and direct Vulkan presentation; Swappy and its Maven/Prefab binary
are not included. Validation binaries used for local testing are not packaged.
