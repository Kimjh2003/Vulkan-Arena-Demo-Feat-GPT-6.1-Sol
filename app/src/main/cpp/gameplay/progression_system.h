#pragma once
#include "combat_system.h"

// Owns wave boundaries, upgrade pause and all run-scoped upgrade rules.
// No dependency on GameSession, JNI or Android UI.
class ProgressionSystem final {
public:
    static constexpr int waveCount=5;
    static constexpr float waveDuration=60.f;
    static constexpr float roundDuration=waveCount*waveDuration;
    bool paused() const noexcept {return awaitingChoice;}
    int wave(float age) const {return std::clamp(1+int(age/waveDuration),1,waveCount);}
    bool choose(int choice,Player& player,CombatSystem& combat) {
        if(!awaitingChoice || choice<1 || choice>3) return false;
        switch(choice) {
            case 1:++combat.damageBonus;break;
            case 2:++combat.rateLevel;break;
            case 3:player.speedBonus+=.15f;break;
        }
        awaitingChoice=false;return true;
    }
    bool advance(float age,bool ended,EnemySystem& enemies,CombatSystem& combat) {
        if(ended || awaitingChoice || nextBoundary>=waveCount || age<nextBoundary*waveDuration) return false;
        ++nextBoundary;awaitingChoice=true;
        enemies.enemies.clear();enemies.shots.clear();
        combat.bullets.clear();combat.hostileBullets.clear();
        combat.ammo=combat.capacity();combat.reload=0;
        return true;
    }
private:
    int nextBoundary=1;
    bool awaitingChoice=false;
};
