#pragma once
#include "enemy_system.h"
#include "weapon.h"
#include "../engine/effects.h"

struct CombatSystem {
    static constexpr int magazineSize=12;
    WeaponKind weapon=WeaponKind::Pistol;
    int damageBonus=0, rateLevel=0;
    void equip(int kind) {weapon=static_cast<WeaponKind>(std::clamp(kind,0,2));ammo=capacity();fire=reload=0;}
    int capacity() const {return weaponSpec(weapon).magazine;}
    int ammo=capacity();
    float fire=0,reload=0,aimX=.5f,aimZ=.4f;
    float directionX=0,directionZ=-1;
    bool hasTarget=false;
    std::vector<Body> bullets,hostileBullets;
    void updateWeapon(float dt,const Player& player,const EnemySystem& system,
                      engine::Effects& effects,bool manual=false,float ax=0,float az=0,bool trigger=false) {
        fire-=dt;
        if(reload>0) {reload=std::max(0.f,reload-dt);if(reload==0) ammo=capacity();}
        hasTarget=false;
        if(manual) {
            const float length=std::hypot(ax,az);
            if(length>.01f) {directionX=ax/length;directionZ=az/length;}
            aimX=player.x+directionX*.12f;aimZ=player.z+directionZ*.12f;
            hasTarget=trigger && length>.01f;
        } else {
            float closest=100;
            for(const auto& enemy:system.enemies) {
                const float distance=std::hypot(enemy.x-player.x,enemy.z-player.z);
                if(enemy.hp>0 && distance<closest && World::clearShot(player.x,player.z,enemy.x,enemy.z)) {
                    closest=distance;aimX=enemy.x;aimZ=enemy.z;hasTarget=true;
                }
            }
            if(hasTarget) {
                const float length=std::max(.001f,std::hypot(aimX-player.x,aimZ-player.z));
                directionX=(aimX-player.x)/length;directionZ=(aimZ-player.z)/length;
            }
        }
        if(hasTarget && fire<=0 && reload==0 && ammo>0) {
            const auto spec=weaponSpec(weapon);
            fire=spec.interval*std::pow(.85f,rateLevel);
            for(int i=0;i<spec.pellets;++i) {
                const float angle=spec.pellets>1?(i-(spec.pellets-1)*.5f)*spec.spread:
                    (weapon==WeaponKind::Smg?std::sin(float(ammo)*2.4f)*spec.spread:0.f);
                const float c=std::cos(angle),s=std::sin(angle);
                bullets.push_back({player.x,player.z,(directionX*c-directionZ*s)*spec.speed,
                                                  (directionX*s+directionZ*c)*spec.speed});
            }
            effects.shot();
            if(--ammo==0) reload=spec.reload;
        }
    }
    int resolve(float dt,Player& player,EnemySystem& system,engine::Effects& effects) {
        int points=0;
        for(auto& bullet:bullets) {
            const float endX=bullet.x+bullet.vx*dt,endZ=bullet.z+bullet.vz*dt;
            auto wall=World::firstHit(bullet.x,bullet.z,endX,endZ,.004f);
            float first=wall?*wall:2.f;Enemy* victim=nullptr;
            for(auto& enemy:system.enemies) {
                if(enemy.hp<=0 || enemy.x<0) continue;
                auto hit=engine::segmentCircle(bullet.x,bullet.z,endX,endZ,enemy.x,enemy.z,.025f);
                if(hit && *hit<first) {first=*hit;victim=&enemy;}
            }
            if(first<=1) {
                const float hitX=bullet.x+(endX-bullet.x)*first,hitZ=bullet.z+(endZ-bullet.z)*first;
                effects.impact(hitX,hitZ,victim!=nullptr);
                if(victim) {victim->flash=.10f;victim->hp=std::max(0,victim->hp-weaponSpec(weapon).damage-damageBonus);if(victim->hp==0) {victim->x=-10;points+=10;}}
                bullet.x=-10;
            } else {bullet.x=endX;bullet.z=endZ;}
        }
        for(const auto& shot:system.shots) if(hostileBullets.size()<240) hostileBullets.push_back(shot);
        for(auto& bullet:hostileBullets) {
            const float endX=bullet.x+bullet.vx*dt,endZ=bullet.z+bullet.vz*dt;
            auto wall=World::firstHit(bullet.x,bullet.z,endX,endZ,.004f);
            auto hit=engine::segmentCircle(bullet.x,bullet.z,endX,endZ,player.x,player.z,.025f);
            if(hit && (!wall || *hit<*wall)) {
                if(player.damage()) effects.hurt(player.x,player.z);
                bullet.x=-10;
            } else if(wall) {
                effects.impact(bullet.x+(endX-bullet.x)* *wall,bullet.z+(endZ-bullet.z)* *wall,false);
                bullet.x=-10;
            } else {bullet.x=endX;bullet.z=endZ;}
        }
        for(auto& enemy:system.enemies) {
            if(enemy.hp>0 && enemy.x>=0 && World::overlaps(player.x,player.z,enemy.x,enemy.z,.03f)) {
                if(player.damage()) effects.hurt(player.x,player.z);
                enemy.x=-10;
            }
        }
        system.removeDead();
        auto prune=[](std::vector<Body>& bodies) {
            bodies.erase(std::remove_if(bodies.begin(),bodies.end(),World::outside),bodies.end());
        };
        prune(bullets);prune(hostileBullets);
        return points;
    }
};
