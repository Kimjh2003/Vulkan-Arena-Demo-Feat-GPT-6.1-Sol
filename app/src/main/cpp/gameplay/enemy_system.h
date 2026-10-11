#pragma once
#include "player.h"
#include <random>
#include <vector>

enum class EnemyKind { Chaser, Charger, Shooter };
struct Enemy : Body {
    EnemyKind kind;
    int hp;
    float cooldown=1.f, windup=0, charging=0, flash=0;
    float chargeX=0, chargeZ=0;
    Enemy(float x,float z,EnemyKind type=EnemyKind::Chaser)
        : Body{x,z},kind(type),hp(type==EnemyKind::Chaser?1:2) {}
};
struct EnemySystem {
    float spawn=0;
    int spawnIndex=0;
    std::vector<Enemy> enemies;
    std::vector<Body> shots;
    std::mt19937 rng{42};
    void spawnEnemies(float dt,float age) {
        spawn-=dt;
        if(spawn>0 || enemies.size()>=120) return;
        spawn=std::max(.22f,.9f-age*.002f);
        const EnemyKind composition[]={EnemyKind::Chaser,EnemyKind::Chaser,EnemyKind::Charger,
                                       EnemyKind::Chaser,EnemyKind::Shooter,EnemyKind::Charger};
        const auto kind=composition[(spawnIndex++)%6];
        const float p=std::generate_canonical<float,24>(rng);
        switch(rng()%4) {
            case 0: enemies.emplace_back(p,0,kind);break;
            case 1: enemies.emplace_back(p,1,kind);break;
            case 2: enemies.emplace_back(0,p,kind);break;
            default: enemies.emplace_back(1,p,kind);
        }
    }
    void chase(float dt,const Player& player) {
        shots.clear();
        for(auto& enemy:enemies) {
            enemy.flash=std::max(0.f,enemy.flash-dt);
            enemy.cooldown-=dt;
            const float dx=player.x-enemy.x,dz=player.z-enemy.z;
            const float distance=std::max(.001f,std::hypot(dx,dz));
            if(enemy.kind==EnemyKind::Charger) {
                if(enemy.windup>0) {
                    enemy.windup=std::max(0.f,enemy.windup-dt);
                    if(enemy.windup==0) {enemy.charging=.48f;enemy.cooldown=2.3f;}
                    continue;
                }
                if(enemy.charging>0) {
                    World::move(enemy.x,enemy.z,enemy.chargeX*dt*.55f,enemy.chargeZ*dt*.55f,.017f);
                    enemy.charging=std::max(0.f,enemy.charging-dt);continue;
                }
                if(enemy.cooldown<=0 && distance<.55f && World::clearShot(enemy.x,enemy.z,player.x,player.z)) {
                    enemy.windup=.6f;enemy.chargeX=dx/distance;enemy.chargeZ=dz/distance;continue;
                }
            }
            if(enemy.kind==EnemyKind::Shooter && (enemy.windup>0 || World::clearShot(enemy.x,enemy.z,player.x,player.z))) {
                if(enemy.windup>0) {
                    enemy.windup=std::max(0.f,enemy.windup-dt);
                    if(enemy.windup==0) {
                        shots.push_back({enemy.x,enemy.z,enemy.chargeX*.5f,enemy.chargeZ*.5f});
                        enemy.cooldown=1.6f;
                    }
                    continue;
                }
                if(enemy.cooldown<=0) {
                    enemy.windup=.45f;enemy.chargeX=dx/distance;enemy.chargeZ=dz/distance;continue;
                }
                if(distance>=.24f && distance<=.38f) continue;
                if(distance<.24f) {World::move(enemy.x,enemy.z,-dx/distance*dt*.09f,-dz/distance*dt*.09f);continue;}
            }
            const auto target=World::waypoint(enemy.x,enemy.z,player.x,player.z);
            const float tx=target[0]-enemy.x,tz=target[1]-enemy.z;
            const float length=std::max(.001f,std::hypot(tx,tz));
            const float speed=enemy.kind==EnemyKind::Chaser?.10f:.075f;
            World::move(enemy.x,enemy.z,tx/length*dt*speed,tz/length*dt*speed,.017f);
        }
    }
    void removeDead() {
        enemies.erase(std::remove_if(enemies.begin(),enemies.end(),
                       [](const Enemy& e){return e.x<0 || e.hp<=0;}),enemies.end());
    }
};
