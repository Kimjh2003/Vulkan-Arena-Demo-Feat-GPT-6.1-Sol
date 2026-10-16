#pragma once
#include "gameplay/game_session.h"
#include <array>
#include <cstdint>
#include <type_traits>
#include <cstring>

// JNI wire format: 14 little/native-endian 32-bit words, 16-byte-aligned storage.
// Dash is an IEEE float bit pattern; all other fields are signed integers.
struct alignas(16) FrameState {
    static constexpr int words=14;
    std::array<std::int32_t,words> values{};
    void capture(const GameSession& g) {
        values={g.score,g.combat.ammo,g.combat.capacity(),g.player.hp,
            std::clamp(int(g.age),0,300),g.wave(),g.manualAim,g.combat.reload>0,
            g.gameOver(),g.victory(),int(g.combat.weapon),g.upgrading(),
            0,int(g.effects.events)};
        static_assert(sizeof(float)==sizeof(std::int32_t));
        std::memcpy(&values[12],&g.player.dashCooldown,sizeof(float));
    }
};
static_assert(sizeof(std::int32_t)==4 && alignof(FrameState)==16);
static_assert(std::is_trivially_copyable_v<FrameState>);
