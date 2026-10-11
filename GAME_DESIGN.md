# 아레나 슈터 — 첫 플레이 버전

## 방향

고정 원근 카메라의 탑다운 3D 생존 슈터. 게임 시간 5분을 버티면 승리한다.
60초 단위로 1~5 웨이브를 표시하며 시간에 따라 적 생성 간격이 짧아진다.
아직 웨이브 사이 상점·강화 선택이나 다른 맵은 없다.

## 조작 설정

메뉴 → 조작 설정에서 선택하고 저장한다. 앱 재실행에도 유지된다.

| 설정 | 동작 |
|---|---|
| 한 손 | 이동 스틱 + 시야가 확보된 가장 가까운 적을 자동 조준/사격 |
| 양손 | 이동 스틱 + 반대쪽 스틱 방향으로 조준/연사; 스틱을 놓으면 사격 정지 |
| 이동 스틱 오른쪽 | 이동/조준 좌우 교환; 한 손 모드에서도 적용 |
| 효과음 | 발사·충돌·플레이어 피해 소리 켜기/끄기 |

두 스틱은 서로 다른 pointer ID를 소유한다. 한 손가락을 떼도 다른 입력은 유지한다.
데드존은 스틱 반경의 16%이며 바깥 입력은 최대값으로 제한한다.
위치는 화면 크기에 비례한다. 자유 배치·크기·투명도 편집은 후속 범위다.

## 적 조합

| 적 | 표시 | 행동 | 체력 |
|---|---|---|---|
| 추적형 | 빨강 | 플레이어에게 접근, 엄폐물을 우회 | 1 |
| 돌진형 | 주황 | 접근 → 약 0.6초 방향 예고 → 고정 방향 돌진 → 재사용 대기 | 2 |
| 원거리형 | 보라 | 거리를 유지하며 시야가 확보될 때 보라색 탄환 발사 | 2 |

생성 패턴은 추적·추적·돌진·추적·원거리·돌진으로 반복한다.
자동 조준은 엄폐물 너머의 적을 제외한다. 돌진 예고 중 바닥 점선으로 방향을 표시한다.
원거리 탄환도 엄폐물에 막힌다. 플레이어는 피해 후 약 0.45초 보호 시간을 갖는다.

## 자리 잡기

맵에 6개의 높이가 있는 엄폐물을 배치했다. 이동 충돌은 원과 AABB,
탄환 충돌은 이동 구간과 AABB/원으로 판정해 빠른 탄환이 얇은 물체를
통과하는 현상을 막는다. 가장 먼저 맞은 엄폐물 또는 적만 처리한다.
벽을 따라 이동할 수 있고 적은 엄폐물 모서리의 우회 지점을 선택한다.
현재는 고정 맵용 간단한 우회 방식이며 복잡한 맵을 위한 내비메시는 아니다.

## 사격감

- 12발 탄창, 0.16초 발사 간격, 탄약 소진 시 1.2초 재장전
- 조준 방향으로 회전하는 총과 캐릭터; 발사 때 총이 뒤로 움직임
- 짧은 총구 섬광, 탄환, 피격/엄폐물 충돌 파편
- 적 피격 시 흰색 반응, 플레이어 피해 시 붉은색 반응
- 발사/피해 때 작은 화면 흔들림
- 직접 합성한 짧은 PCM WAV 효과음 3종을 SoundPool로 재생

## 3D 엔진 범위

기존 GLES 기반의 **자체 경량 3D 런타임**을 확장했다.
외부 상용 엔진을 도입하지 않았다. `engine/collision.h`는 Vec3/AABB와 연속
충돌 검사를, `engine/effects.h`는 수명 있는 3D 입자·화면 반응·효과 이벤트를 담당한다.
GlesRenderer는 회전 가능한 박스 메시·면 조명·원근 카메라를 렌더링한다.
그림자는 접지 표현용 단순 도형이며 실시간 그림자 맵은 아니다.
게임 이동은 XZ 지면에 제한된다. 스켈레탈 애니메이션·래그돌·범용 강체 물리·모델
임포터는 아직 없으며, 필요할 때 해당 모듈을 확장한다.

GLES 3.2 → 3.1 → 3.0 순서의 컨텍스트 선택과 후처리 호환 경로는 유지한다.
Android 9~17, JDK 25 빌드 설정도 유지한다. 실기기 3.1/3.2 검증은 후속 범위다.

## Progression and player choices

Choose a starting weapon on the home screen. Pistol: 12 rounds, 0.22s interval, 1.2s reload. Shotgun: 6 rounds, 5 pellets per shot, 0.65s interval, 1.6s reload. SMG: 30 rounds, 0.085s interval, 1.45s reload and slight spread. Each projectile starts at 1 damage.

Dash uses current movement direction or the last direction when idle. Duration 0.18s, cooldown 2.5s; it briefly protects against damage and still collides with cover.

At 60/120/180/240 simulation seconds, combat pauses and threats are cleared. The magazine refills. Choose damage +1, firing interval multiplied by 0.85, or movement speed +15 percentage points. Choices stack for that run; retry resets upgrades.

Shooters show a purple dotted line for 0.45s and fire along the locked direction. Chargers show an orange line during their existing 0.6s windup. Cover blocks their attacks.

Completed rounds save independent best score/time and victory count using local preferences. Quitting an unfinished round does not submit a record. Starting weapon selection persists. No online leaderboard.
