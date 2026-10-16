> Original archive validation notes. Current Android migration verification is in `docs/VULKAN_MIGRATION.md` at the repository root.

# 検証 / 검증

- C++17 전체 API 소스 compile/link: -Wall -Wextra -Wpedantic -Werror 통과.
- CPU 테스트: ABI, 버전 협상, Vulkan near/far depth, GLES 카메라와 screen→ground→screen 좌표 일치, texture family 대체, compressed mip 크기·단계, timestamp wrap 통과.
- arena.vert / arena.frag / copy.comp: libshaderc로 Vulkan1.0 / SPIR-V1.0 컴파일 통과.
- CMake 구성은 제공했지만 이 환경에 CMake 실행기가 없어 직접 C++ compiler로 검증했다.
- GPU driver validation / 렌더링 이미지 / Android 빌드·실행은 미검증. GPU 실행 성공을 주장하지 않는다.
