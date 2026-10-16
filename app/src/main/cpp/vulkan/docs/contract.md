# 렌더링 계약

## 입력 근거

첨부 APK SHA-256: `ad58ff7cedac8d77bd638282e9ba5c997063225bb3ebe7e8dbe012e4cd58f359`.
동일 바이너리의 내장 GLSL, GL imports, 렌더링 관련 문자열을 분석하고 사용자가 제공한 `Kimjh2003/gles-arena-demo`의 scene_pipeline·material_texture·post_processor 소스로 ABI와 수식을 확인했다. APK에서 원본 C++를 그대로 복원한 결과물은 아니다. 저장소나 게임 앱을 수정하지 않고 독립 Vulkan 소스로 작성했다.

## GLES → Vulkan

| GLES 기능 | Vulkan 구현 |
|---|---|
| VBO/VAO + glVertexAttribDivisor | 2개 vertex binding, VERTEX/INSTANCE rate |
| glDrawArraysInstanced | vkCmdDraw(36, instanceCount, 0, 0) |
| FrameData UBO | set 0 binding 0, frame slot마다 독립 buffer |
| floorMap | set 0 binding 1, 압축 자산/밉 sampler |
| depth FBO shadow | slot마다 깊이 image/view/framebuffer, depth-only pipeline |
| sampler2DShadow + 3×3 PCF | sampled depth + 9회 수동 비교, nearest sampler |
| glCompressedTexImage2D | compressed VkFormat + staging + vkCmdCopyBufferToImage |
| ASTC/ETC 선택 | enabled feature + format/usage/extent/mip 지원 조회 |
| GL anisotropy extension | 코어 samplerAnisotropy를 활성화했을 때만 사용 |
| GLES3.1 compute copy | SPIR-V1.0 8×8 compute + storage image + barrier |
| fragment/transfer diagnostic copy | 같은 format의 vkCmdCopyImage로 동등한 원본 픽셀 복사 |
| EXT disjoint timer query | timestampValidBits/period + nonblocking availability 조회 |
| KHR_debug callback | 대응 debug-utils는 확장이므로 코어 전용 API에 포함하지 않음 |
| EGL/surface/present | 이 패키지 범위 밖, 호출자가 관리 |

원래 APK의 기본 Direct 출력처럼 scene은 호출자가 제공한 최종 color+depth target으로 바로 기록한다. 그림자에는 별도 깊이 target을 사용한다. 선택적 ComputeCopy/recordDiagnosticCopy는 기본 scene.record가 호출하지 않는다. 안개·발광은 셰이더 수식이며 블룸·톤매핑은 추가하지 않았다.

## 호출과 수명

1. 호출자가 Vulkan instance/device/queue, command pool, command buffers, color+depth renderpass/framebuffers, 슬롯별 fence를 만든다.
2. 실제 device creation에서 활성화한 ASTC_LDR/ETC2/samplerAnisotropy 값을 TextureStore에 전달한다. 지원만 조회하고 활성화하지 않은 기능을 true로 전달하면 안 된다.
3. graphics queue family의 recording primary command buffer에서 TextureStore.upload를 호출한다. renderpass 밖이어야 한다. payload는 호출 중 복사되며 원본 bytes는 이후 해제할 수 있다.
4. 업로드 제출·완료를 기다린 뒤 releaseStaging을 호출하고 sampled(id)를 얻는다. 같은 queue 또는 적절한 semaphore/acquire barrier로 업로드 완료를 scene에 연결한다.
5. ScenePipeline 생성 시 제공한 shaders/arena.vert.spv, arena.frag.spv를 uint32_t 벡터로 읽는다. borrowed color renderpass는 subpass0, color attachment0 + depth attachment1, samples=1이다. CLEAR loadOp 및 올바른 초기/최종 layout과 외부 의존성은 호출자가 정의한다. framebuffer/extent/attachment format은 해당 renderpass와 호환되어야 한다.
6. 해당 슬롯의 이전 제출 fence를 wait하고, 그 signaled fence를 record에 전달한다. command buffer를 먼저 reset/begin한다. API는 vkGetFenceStatus로 완료 여부를 검사한다. 다른 슬롯 fence를 전달하거나, 아직 제출하지 않은 동일 슬롯을 재기록하면 안 된다. 슬롯별 command buffer를 사용하고 한 제출에서 같은 슬롯은 한 번만 기록한다.
7. record 완료 후 caller가 command buffer를 end하고 fence를 reset한 뒤 submit한다. record 전에 fence를 reset하면 signaled 검사에 실패한다. queue 제출/표시/WSI는 API가 하지 않는다.
8. TextureStore·ScenePipeline·ComputeCopy 파괴 전에 참조하는 모든 GPU 작업을 완료한다. device와 borrowed renderpass는 그 뒤에 파괴한다. destructor가 waitIdle을 호출하지 않는다.

record는 해당 command buffer의 graphics binding 상태를 변경한다. 후속 draw는 자신이 사용하는 상태를 다시 설정해야 한다. CPU 리소스 변경·명령 기록·파괴는 외부에서 직렬화한다. VulkanError는 원래 VkResult를 보존한다. device lost/OOM은 호출자에게 전달하며 무한 재시도하지 않는다.

## 동기화와 예외

- Texture upload: UNDEFINED → TRANSFER_DST → SHADER_READ_ONLY, transfer write → fragment read. 이 업로드를 ComputeCopy에서 샘플링하면 별도 fragment-stage 범위로 끝난 업로드를 compute-stage에도 보이게 연결해야 한다(ComputeCopy 입력 stage/access를 전달).
- Shadow: 이전 fragment read → depth write → 현재 fragment read. 슬롯마다 독립 shadow image와 UBO/instance buffer/descriptor set을 사용한다.
- Shadow 지원: D32_SFLOAT, D16_UNORM의 sampled+depth usage와 extent를 조회한다. 둘 다 미지원이거나 shadowSize=0이면 그림자만 비활성화한다. OOM/device lost는 지원 부족과 혼동하지 않고 예외로 전달한다.
- PCF: 원래 GLES hardware compare/filter와 시각적 결과가 완전히 동일하다고 보장하지 않는다. 이식본은 9회 nearest 수동 깊이 비교로 코어 호환성을 확보한다.
- Texture: preferred 지원 시 사용, 아니면 ASTC→ETC2→RGBA8. 사용할 family의 payload가 있어야 하며 비압축 대체 데이터를 자동 디코딩/생성하지 않는다. 밉은 연속적인 절반 크기이고 1×1 뒤에 추가 레벨이 없어야 한다. 불완전한 밉 체인은 maxLod를 마지막 입력 레벨로 제한한다. malformed payload는 지원 미달과 다르게 오류로 처리한다.
- ASTC 압축 payload의 블록 문법·LDR 여부는 자산 생성 측 계약이다. parseAstc는 파일 헤더·2D 크기·블록 payload 길이를 검사하며 full decoder가 아니다. TextureAsset.astc4x4에는 .astc 헤더를 제외한 4×4 블록만 전달한다.
- samplerAnisotropy는 device에서 활성화했을 때만 사용하며 maxSamplerAnisotropy 이하로 제한한다. 포맷에 linear filter가 없으면 nearest + anisotropy off.
- 인스턴스 개수가 maxInstances를 넘으면 예외이며 GPU가 사용하는 buffer를 실행 중에 재할당하지 않는다.
- extent는 framebuffer/viewport 한도로 검사한다. nonfinite 좌표·색·투영, 비양수 박스 크기, 단위 cos/sin이 아닌 rotation은 예외이다.
- ComputeCopy는 GRAPHICS+COMPUTE queue family, RGBA8_UNORM storage target, sampled source와 일치하는 mip0/layer0 view를 요구한다. source/destination은 서로 다른 이미지이고 메모리가 겹치면 안 된다. usage/실제 extent는 호출자가 보장한다. 이 객체의 슬롯도 signaled fence 이후에만 업데이트한다. storage 포맷 미지원이면 진단 copy 객체를 만들지 않거나 transfer copy로 대체한다.
- DiagnosticCopy는 같은 포맷의 TRANSFER_SRC/DST 지원 이미지 간 mip0/layer0 복사이다. 해당 image layout/stage/access 및 queue ownership을 호출자가 정확히 전달한다. source layout을 복원하고 destination을 지정 final layout으로 전환한다.
- timestamp helper는 코어 TIMESTAMP query pool과 해당 graphics queue의 timestampValidBits>0을 요구한다. reset은 renderpass 밖에서 기록한다. query indices는 완료된 슬롯 전용으로 사용한다. poll은 WAIT flag 없이 availability를 확인한다. device timestampPeriod로 ms를 계산하고 validBits에 따른 wrap을 처리한다. 측정 구간은 한 wrap보다 짧아야 한다. 입력 지연을 측정하는 API가 아니다.

## 좌표·버전

월드 X/Y/Z는 좌우/높이/깊이, 카메라는 (0,7,7), vertical FOV=50°, near=.1, far=50. 양수 viewport height로 Vulkan1.0 호환을 유지하고 vertex shader에서 Y를 뒤집는다. depth는 GLES [-1,1]에서 Vulkan [0,1]로 변환한다. shadow UV도 Vulkan viewport Y 방향에 맞췄다. screenToGround는 월드 좌표만 반환하며 게임 arena clamp는 포함하지 않는다.

core API baseline=1.0, shaders=SPIR-V1.0. subgroup/wave 크기, 타일/캐시 구조, vendor extension, dynamic rendering, synchronization2를 요구하지 않는다. 최신 버전/세대 이름만으로 포맷이나 선택 feature를 가정하지 않는다. Android WSI 확장은 로컬 앱이 별도로 처리할 범위이다.

## Android integration adaptations

ScenePipeline.record accepts a mutable reusable vector and partitions it in place.
Frame buffers remain mapped, descriptors only update on a texture change, and
limits are cached. Context.uploadQueue/family optionally stage static cube data
into device-local preferred memory at construction. The Android integration adds
WSI, presentation and lifecycle handling; see the repository migration document.
