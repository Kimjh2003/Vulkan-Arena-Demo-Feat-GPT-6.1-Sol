#pragma once
#include "progression_system.h"

// Owns one round and the update order. Presentation consumes this state read-only.
struct GameSession {
    Player player;
    EnemySystem enemySystem;
    CombatSystem combat;
    engine::Effects effects;
    bool manualAim=false;
    float age = 0;
    int score = 0;
    ProgressionSystem progression;
    void reserveRuntimeMemory() {
        enemySystem.enemies.reserve(120);enemySystem.shots.reserve(120);
        combat.bullets.reserve(128);combat.hostileBullets.reserve(240);
        effects.particles.reserve(160);
    }
    bool upgrading() const noexcept {return progression.paused();}
    bool chooseUpgrade(int choice) {return progression.choose(choice,player,combat);}
    void checkWave() {progression.advance(age,gameOver() || victory(),enemySystem,combat);}
    bool gameOver() const { return !player.alive(); }
    bool victory() const {return age>=ProgressionSystem::roundDuration && player.alive();}
    int wave() const {return progression.wave(age);}
    void update(float dt, float targetX, float targetZ, bool touching) {
        dt = std::clamp(dt, 0.f, .05f);
        effects.beginFrame(dt);
        if (gameOver() || victory() || upgrading()) return;
        age += dt;
        player.tick(dt);
        player.move(dt, targetX, targetZ, touching);
        enemySystem.spawnEnemies(dt, age);
        combat.updateWeapon(dt, player, enemySystem,effects);
        enemySystem.chase(dt, player);
        score += combat.resolve(dt, player, enemySystem,effects);
        checkWave();
    }
    void updateControls(float dt,float moveX,float moveZ,float aimX,float aimZ,bool shooting) {
        dt=std::clamp(dt,0.f,.05f);
        effects.beginFrame(dt);
        if(gameOver() || victory() || upgrading()) return;
        age+=dt;
        player.tick(dt);
        player.moveStick(dt,moveX,moveZ);
        enemySystem.spawnEnemies(dt,age);
        combat.updateWeapon(dt,player,enemySystem,effects,manualAim,aimX,aimZ,shooting);
        enemySystem.chase(dt,player);
        score+=combat.resolve(dt,player,enemySystem,effects);
        checkWave();
    }
};
