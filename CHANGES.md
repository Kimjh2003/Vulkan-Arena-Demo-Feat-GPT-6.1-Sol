## 2026-10-12 — 무기 선택과 강화 규칙 분리

- WeaponPicker.kt로 무기 선택 버튼/설명/다이얼로그 분리. MainActivity는 선택 저장과 메뉴 갱신만 담당.
- ProgressionSystem으로 웨이브 상수/경계/정지 상태/강화 유효성/누적/전환 정리 및 탄창 보충 규칙 분리.
- GameSession은 진행 모듈을 소유하고 라운드 업데이트 순서 및 위임만 담당.
- 기존 무기 수치, 선택 항목, 기록 저장, JNI 상태 키와 조작 동작 유지.

## 2026-10-12 — 전투 선택과 진행 기능 5종

- 대시: 버튼, 이동 방향/마지막 방향으로 0.18초 이동, 2.5초 쿨다운, 짧은 피해 보호, 엄폐물 충돌 유지.
- 무기: 홈 선택/저장. 권총 12발/0.22초, 산탄총 6발/5개 산탄/0.65초, 기관단총 30발/0.085초 및 작은 확산. 자동/수동 공통.
- 업그레이드: 게임 시간 60/120/180/240초에 전투 정지, 적/탄환 정리와 탄창 보충 후 공격력+1/발사 간격-15%/이동 속도+15% 중 하나 선택. 이번 판에 누적.
- 공격 예고: 돌진형의 방향 표시 강화, 원거리형 0.45초 조준 예고 후 고정 방향 발사.
- 기록: 완료된 판의 최고 점수, 최장 생존 시간, 성공 횟수를 로컬 저장. 무기 선택도 유지.
- 기존 직접 렌더링/전체 화면/코너 HUD/결과 화면 유지.
- C++ 회귀 검사: 무기 탄창/산탄, 대시 쿨다운/피해 보호/벽 충돌, 강화 정지/유효 선택/중복 방지/누적 및 실제 피해 적용 통과.
- arm64/x86_64 APK와 검사 APK 빌드 통과. 에뮬 통합 검사: 강화 선택/패널 닫기, 최고 기록 유지, 무기 저장, HUD/조작/홈 종료 통과. S24 신규 기능 검증은 아직.


## 2026-10-12 — 직접 렌더링 및 GLES 공통 모듈

- 기본 출력 경로를 화면 framebuffer(0) 직접 렌더링으로 변경.
- 기본 경로에서는 PostProcessor, RGBA8 중간 텍스처 2개, 별도 깊이 버퍼, 전체 화면 복사/블릿을 생성하거나 실행하지 않음.
- `graphics::FrameOutput`: 직접 출력과 선택적 `DiagnosticCopy` 경로의 생성/시작/종료/해제 책임을 통합.
- `graphics::createProgram`: 장면/fragment 복사/compute 복사 셰이더의 컴파일·링크·진단·임시 자원 정리를 통합.
- 기존 GLES 3.2→3.1→3.0 컨텍스트/GLSL 선택 및 ASTC 유틸리티 유지.
- 진단용 복사 경로는 GlesRenderer 생성 인자로 명시 선택하며 기본 플레이에서는 비활성.
- 실제 S24 프레임 시간/입력 지연 개선 수치는 아직 측정하지 않음.

## 2026-10-12 — 작은 코너 HUD와 결과 화면

- 왼쪽 위에 점수/시간, 탄약/체력, 웨이브/조작을 3줄로 압축.
- 재장전 중에만 추가 상태 문구 표시. 전체 너비 카드 제거.
- 사망 시 배경을 어둡게 하고 중앙에 큰 게임 오버와 최종 점수/시간/웨이브 표시.
- 결과 화면은 게임 터치를 차단하며 다시 하기/홈으로 버튼 제공.
- 5분 생존 성공도 동일한 결과 화면 사용. 종료 상태는 한 번만 UI에 전달.
- 결과 화면이 떠 있는 상태에서 앱 복귀 시 자동으로 새 라운드 시작하지 않음.

## 2026-10-12 — 전체 화면과 오버레이 HUD

- SurfaceView와 가상 스틱은 화면 전체 크기로 배치.
- 상단 HUD 전용 높이/불투명 배경을 제거하고 게임 위에 상태 카드를 오버레이.
- HUD 카드는 화면 너비에 맞게 분배해 오른쪽의 빈 상단바 공간 제거.
- 카메라 컷아웃은 HUD 위치에만 반영하고 게임 배경은 화면 끝까지 렌더링.
- 홈도 몰입형 전체 화면 적용. 시스템 바는 가장자리 스와이프로 일시 표시 가능.

## 2026-10-12 — 전용 현황 패널과 홈 종료

- 플레이 영역과 분리한 상단 HUD: 점수, 탄약, 체력, 생존 시간, 웨이브, 조작 모드.
- 재장전/낮은 탄약/낮은 체력/승리/게임 오버를 문구와 의미 색상으로 표시.
- JNI는 숫자/불리언 JSON 상태를 전달하며 표시 문자열 파싱에 의존하지 않음.
- 게임 중 뒤로는 홈, 홈에서 뒤로는 Activity 종료. Android 13+ 콜백을 명시적으로 활성화.
- 시스템 바/디스플레이 컷아웃 인셋 처리, 큰 글꼴/좁은 화면에서는 HUD 가로 스크롤.
- CopilotKit/OpenIntelligentUI의 MIT 디자인 토큰을 네이티브 Kotlin 테마로 적용.
- APK에 원본 MIT 고지 포함. 범위와 출처는 THIRD_PARTY_NOTICES.md 참조.
- 기존 버전의 S24 실기기 구동은 사용자 영상으로 확인. 이번 UI 패치의 S24 검증은 별도.
- JDK 25 arm64/x86_64 APK 빌드, C++ 게임 회귀 검사 통과.
- Android 16 GLES 3.0 에뮬: 입력·설정·JSON HUD·플레이 뒤로·홈 종료 통합 검사 통과.
- 실제 화면에서 6개 상태 카드, 체력 경고, 게임/스틱 영역 분리 확인 (`app/build/outputs/hud-panel.png`).
- 에뮬 부팅 중 System UI ANR이 발생해 시스템 UI를 재시작하고 해상도를 낮춰 검증. 앱 기능 검사는 이후 통과.

## 2026-10-11 — 아레나 슈터 첫 플레이 버전

- 한 손 자동 조준/사격, 양손 직접 조준/사격, 독립 멀티터치 가상 스틱.
- 메뉴 조작 설정: 모드·스틱 좌우·효과음 저장 및 재실행 유지.
- 자체 경량 엔진 계층: Vec3/AABB, 이동 구간 충돌, 3D 입자와 피드백 이벤트.
- 6개 엄폐물, 이동/탄환/시야 차단, 적의 모서리 우회.
- 추적형·돌진형·원거리형 적, 돌진 예고, 적 탄환 및 적별 체력.
- 총/캐릭터 회전, 총구 섬광, 시각적 반동, 피격 파편·색 반응·화면 흔들림.
- SoundPool 효과음 3종 및 켜기/끄기, 백그라운드 정지·자원 해제.
- 게임 시간 5분 생존 목표, 시간에 따른 생성 간격 조정, 웨이브 표시.
- JDK 25 arm64/x86_64 debug APK 및 입력 검사 APK 빌드 통과.
- 확장한 게임 회귀 검사: 수동 사격, 엄폐물, 빠른 탄환, 적 행동, 우회, 피해 보호, 승리 통과.
- Android 입력 통합 검사: 멀티터치·개별 해제·취소·모드·좌우 교환·설정 저장 통과.
- GLES 3.0 에뮬레이터에서 두 모드 화면/사격, 저장 후 프로세스 재시작 유지 확인.
- Arena/AndroidRuntime/SoundPool 오류 로그 없음. 부팅 중 에뮬레이터 런처/System UI ANR는 발생.
- 물리 기기 3.1/3.2, 오디오 품질/지연 검증은 후속 범위.
- GAME_DESIGN.md와 MODULES.md, README.md에 현재 구현 범위 반영.

## 2026-10-11 — 기능 모듈 분리

- 게임 상태·규칙: GameSession, Player, EnemySystem, CombatSystem, World.
- 표현: SceneBuilder, HudPresenter, Camera3D.
- GPU: GlesRenderer와 PostProcessor의 자원·역할 분리.
- Kotlin: TouchInput, GameUi 분리; MainActivity는 앱 수명·스레드 연결 담당.
- CMake: arena_gameplay, arena_graphics, arena JNI 대상으로 구성.
- MODULES.md에 책임·API·의존 관계·미구현 에셋/오디오 확장 위치 정의.
- NDK Android x86_64 게임 회귀 검사 실행 통과.
- JDK 25 arm64/x86_64 APK 빌드 통과; GLES 3.0 에뮬레이터 렌더링/HUD/자동 사격 확인.
- 에뮬레이터 System UI ANR가 나타났지만 앱의 Arena/AndroidRuntime 오류는 기록되지 않음.

## 2026-10-11 — GLES 3.2-first compatibility

- Context negotiation tries 3.2, 3.1, 3.0, then legacy ES 3 creation attributes.
- Actual context selects GLSL ES 3.20/3.10/3.00.
- ES 3.1+ uses compute; ES 3.0 uses full-screen fragment copy with matching texelFetch.
- Compute capability queries and API calls are guarded; manifest requires ES 3.0.
- ASTC is selected using separately advertised compressed formats.
- JDK 25 debug build passed for arm64-v8a and x86_64.
- GLES 3.0 emulator: context fallback, fragment postprocess and gameplay rendering verified.

## 2026-10-11 — GLES 3.2 / ASTC port

- Explicit EGL 3.2 context, runtime version check, GLSL ES 3.20 and manifest 0x00030002.
- ASTC GLES capability/format policy, validated complete mip uploads and optional Mask UNORM8 decode.
- GLES compute copy postprocess connected to scene framebuffer and EGL presentation.
- Apache-2.0 license retained in source and packaged assets.
- JDK 25 debug build passed for arm64-v8a and x86_64.
- GLES 3.0 emulator: context fallback, fragment postprocess and gameplay rendering verified.
- Current software emulator supports only GLES 3.0; 3.2 runtime and hardware ASTC checks remain pending.

# 수정본 변경 사항

## 3D 아레나 전환

- 2D 사각형 렌더링을 깊이 버퍼와 원근 카메라를 사용하는 3D 박스 메시로 교체
- 바닥, 격자, 경계, 플레이어, 적, 탄환과 조준 표시를 3D 월드에 배치
- 터치 화면 좌표를 카메라 광선으로 지면의 XZ 좌표에 변환
- 게임 내부 좌표를 XZ 평면으로 정리하고 기존 전투 규칙 유지
- 플레이 중 시스템 바를 숨겨 HUD가 상태 표시줄과 겹치지 않도록 조정
- x86_64 에뮬레이터 빌드 추가, Android 16 에뮬레이터에서 렌더링과 드래그 이동 확인

## Kotlin 및 Android 9~17 마이그레이션

- `MainActivity.java`를 `MainActivity.kt`로 전환하고 JNI 연결을 유지
- Android 9~17 지원: `minSdk 28`, `compileSdk` 및 `targetSdk 37`
- JDK 25 실행용 Gradle 9.6 wrapper 및 Android Gradle Plugin 9.3.1 적용
- AGP 내장 Kotlin 컴파일 사용, 별도 Kotlin 플러그인 제거
- Android SDK NDK 30.0.15729638 및 CMake 4.1.2 설정
- JDK 25 환경에서 debug APK 빌드 확인

- OpenGL ES 3.0 전용 렌더러: EGL, 셰이더, VBO/VAO, 삼각형 일괄 렌더링
- 게임 화면: 자동 조준 십자선, 탄약, 체력, 점수
- 탄창 12발, 탄약 소진 시 1.2초 자동 재장전
- 게임 화면에서 FPS·프레임 시간·드라이버 정보 제거
- 드라이버 정보와 상세 오류는 Arena 태그로 Logcat 기록
- 직접 Vulkan 호출 및 링크 의존성 제거

## 주요 수정 파일

- app/src/main/cpp/game.h: 조준, 탄약, 재장전, 게임 로직
- app/src/main/cpp/native.cpp: 게임과 렌더러 연결, 게임 HUD
- app/src/main/cpp/gles_renderer.h: 렌더러 API
- app/src/main/cpp/gles_renderer.cpp: GLES 구현 및 개발 로그
- app/src/main/cpp/render_types.h: 도형 데이터
- app/src/main/cpp/CMakeLists.txt: 네이티브 빌드 설정
- app/src/main/java/dev/demo/arena/MainActivity.java: 메뉴, 입력, 화면 수명 관리
- app/src/main/AndroidManifest.xml: Android 앱 설정
- tests/game_test.cpp: 게임 로직 검사
- README.md: 빌드 방법 및 구현 범위

## 검증 상태

호스트 게임 로직 검사 및 AddressSanitizer/UndefinedBehaviorSanitizer 검사는 통과했습니다.
환경 제약으로 LeakSanitizer는 비활성화했습니다.
Android SDK/NDK가 없어 APK 빌드 및 Android 기기 실행은 검증하지 못했습니다.
이 ZIP은 전체 소스 프로젝트이며 APK와 Gradle wrapper는 포함하지 않습니다.
