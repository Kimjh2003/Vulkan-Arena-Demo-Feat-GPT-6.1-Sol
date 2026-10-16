# S24 Sokatoa 검증 — 2026-10-14

## 환경 및 결과

- Sokatoa 1.1.0, USB 연결 SM-S921N, Android 17/API 37, Samsung Xclipse 940.
- 저장 프로파일: `C:/Users/kimjh/sokatoa/profiles/arena-kotlin-r8-S24/profile.sokatoa`.
- 약 5초 캡처: Vulkan GFXR 5,190,534 bytes, Perfetto 6,736,307 bytes.
- Sokatoa에서 시스템/성능 데이터를 열었고, 새 프로파일의 GFXR 재생 및 프레임 이미지 생성이 성공했다.
- 프로파일 빌드는 R8 실행 + RelWithDebInfo/ThinLTO/NDEBUG 네이티브 빌드이며 APK만 debuggable이다. 디버그 가능 APK의 Kotlin 최적화/DEX는 비디버그 배포 APK와 같다고 보장하지 않는다.

## 호출 간격 분석

Perfetto를 SQLite로 내보낸 뒤 실제 Vulkan 호출을 하는 ArenaRenderer TID 3789의 vkQueuePresentKHR 시작 시각만 사용했다. 다른 프로세스와 드라이버 보조 스레드는 제외했다.

| 항목 | 값 |
|---|---:|
| 표시 호출 수 | 618 |
| 호출 간격 중앙값 | 8.331 ms |
| 호출 간격 95% 지점 | 9.786 ms |
| 최대 간격 | 92.358 ms |
| 16.7ms 초과 간격 | 1 / 617 |

최대 간격은 추적 시작 약 11.5ms 지점에 있으며, 캡처 시작 전 프레임 8183의 vkQueuePresentKHR에서 약 91.6ms가 걸렸다. 캡처 경계와 겹치는 지연으로 보이지만, 일반 실행에서 동일한 지연이 없다는 증명은 아니다. 호출 간격은 실제 디스플레이 표시 시각/FPS/터치 지연과 다르다.

Sokatoa가 보고한 평균 command buffer duration은 약 0.971ms다. Perfetto importer는 clock snapshot/unknown clock 및 신규 event/field 경고를 보고했으므로 CPU/GPU 교차 시각이나 모든 카운터의 정확성을 보증하지 않는다. 5초 캡처로 장시간 발열/메모리 안정성을 판단하지 않는다.

## 발견한 도구 호환성 문제와 대응

일반 Debug 앱은 노출된 Khronos validation layer를 자동 활성화하며 VK_EXT_validation_features를 요청한다. Sokatoa 재생이 해당 layer를 제거한 뒤 확장을 지원하지 못해 첫 프로파일에서 vkCreateInstance / VK_ERROR_EXTENSION_NOT_PRESENT가 발생했다. 앱 실제 실행은 성공했다.

`profile` 빌드 타입을 추가했다. 캡처에 필요한 debuggable은 허용하고, native build는 RelWithDebInfo/NDEBUG로 검증 전용 확장을 제외한다. 새 캡처에서 재생 성공을 확인했다. 배포 benchmark/release 설정은 변경하지 않았다.

빌드: `./gradlew.bat :app:assembleProfile`.
최종 폰에는 `releases/VulkanArena-0.2-kotlin-modular.apk`를 다시 설치했다. Sokatoa 캡처 종료 후 전역 GPU debug 설정 enable_gpu_debug_layers/gpu_debug_app은 null이었다.
