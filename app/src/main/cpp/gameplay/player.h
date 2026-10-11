#pragma once
#include "world.h"

struct Player {
    static constexpr int maxHealth = 5;
    float x = .5f, z = .5f;
    int hp = maxHealth;
    float invulnerability = 0;
    float speedBonus=1, dashRemaining=0, dashCooldown=0, dashX=0, dashZ=-1;
    float lastX=0,lastZ=-1;
    bool dash(float dx,float dz) {
        if(!alive() || dashCooldown>0) return false;
        float length=std::hypot(dx,dz);
        if(length<.01f) {dx=lastX;dz=lastZ;length=1;}
        dashX=dx/length;dashZ=dz/length;
        dashRemaining=.18f;dashCooldown=2.5f;
        invulnerability=std::max(invulnerability,.18f);return true;
    }
    void tick(float dt) {invulnerability=std::max(0.f,invulnerability-dt);dashCooldown=std::max(0.f,dashCooldown-dt);}
    bool alive() const { return hp > 0; }
    void move(float dt, float targetX, float targetZ, bool touching) {
        if (!touching) return;
        const float dx = targetX - x, dz = targetZ - z;
        const float distance = std::hypot(dx, dz);
        const float step = distance > 0 ? std::min(1.f, dt * .65f / distance) : 0;
        World::move(x,z,dx*step,dz*step);
        World::constrainPlayer(x, z);
    }
    void moveStick(float dt,float dx,float dz) {
        const float length=std::hypot(dx,dz);
        if(dashRemaining>0) {
            float step=std::min(dt,dashRemaining);dashRemaining=std::max(0.f,dashRemaining-dt);
            World::move(x,z,dashX*step*1.5f,dashZ*step*1.5f);
            World::constrainPlayer(x,z);return;
        }
        if(length>.01f) {lastX=dx/length;lastZ=dz/length;}
        if(length>1) {dx/=length;dz/=length;}
        World::move(x,z,dx*dt*.38f*speedBonus,dz*dt*.38f*speedBonus);
        World::constrainPlayer(x,z);
    }
    bool damage() {
        if(invulnerability>0 || !alive()) return false;
        hp=std::max(0,hp-1);invulnerability=.45f;return true;
    }
};
