#pragma once
#include "gameplay/game_session.h"
#include "render_types.h"

// Converts simulation state to drawing primitives. Never changes gameplay.
struct SceneBuilder {
    static std::vector<Box> build(const GameSession& game) {
        std::vector<Box> out;
        build(game,out);
        return out;
    }
    static void build(const GameSession& game,std::vector<Box>& out) {
        buildStatic(out);
        appendDynamic(game,out);
    }
    static void buildStatic(std::vector<Box>& out) {
        out.clear();
        out.reserve(80);
        out.push_back({-4.f,-.18f,-4.f,8.f,.18f,8.f,.07f,.12f,.19f});
        out.back().textured=true;
        for (int i=1;i<20;i++) {
            float p=-4.f+i*.4f;
            out.push_back({p-.008f,.002f,-4.f,.016f,.006f,8.f,.10f,.19f,.27f});
            out.push_back({-4.f,.002f,p-.008f,8.f,.006f,.016f,.10f,.19f,.27f});
        }
        out.push_back({-4.04f,0,-4.04f,8.08f,.15f,.08f,.19f,.34f,.44f});
        out.push_back({-4.04f,0,3.96f,8.08f,.15f,.08f,.19f,.34f,.44f});
        out.push_back({-4.04f,0,-4.f,.08f,.15f,8.f,.19f,.34f,.44f});
        out.push_back({3.96f,0,-4.f,.08f,.15f,8.f,.19f,.34f,.44f});
        for(const auto& cover:World::cover) {
            const float w=cover.max.x-cover.min.x,d=cover.max.z-cover.min.z;
            out.push_back({cover.min.x-.05f,.008f,cover.min.z-.05f,w+.1f,.01f,d+.1f,.025f,.04f,.06f});
            out.push_back({cover.min.x,0,cover.min.z,w,cover.max.y,d,.30f,.38f,.44f});
            out.push_back({cover.min.x,cover.max.y,cover.min.z,w,.025f,d,.47f,.58f,.62f});
        }
    }
    template<class Output>
    static void buildDynamic(const GameSession& game,Output& out) {
        out.clear();
        appendDynamic(game,out);
    }
    template<class Output>
    static void appendDynamic(const GameSession& game,Output& out) {
        auto world=[](float p) { return World::toWorld(p); };
        for (const auto& e:game.enemySystem.enemies) {
            const auto color=e.flash>0?engine::Vec3{1,1,1}:
                e.kind==EnemyKind::Charger?engine::Vec3{1,.58f,.09f}:
                e.kind==EnemyKind::Shooter?engine::Vec3{.65f,.24f,1}:engine::Vec3{1,.18f,.29f};
            const float h=e.kind==EnemyKind::Shooter?.6f:.42f;
            out.push_back({world(e.x)-.18f,.012f,world(e.z)-.18f,.36f,.01f,.36f,.025f,.04f,.06f});
            out.push_back({world(e.x)-.13f,0,world(e.z)-.13f,.26f,h,.26f,color.x,color.y,color.z});
            out.push_back({world(e.x)-.10f,h,world(e.z)-.10f,.20f,.13f,.20f,color.x*.8f,color.y*.8f,color.z*.8f});
            if(e.windup>0) {
                for(int i=1;i<=10;++i) out.push_back({world(e.x)+e.chargeX*i*.3f-.035f,.022f,
                    world(e.z)+e.chargeZ*i*.3f-.035f,.07f,.018f,.07f,1,e.kind==EnemyKind::Shooter?.15f:.65f,e.kind==EnemyKind::Shooter?1.f:.12f});
            }
        }
        for (const auto& b:game.combat.bullets) {
            out.push_back({world(b.x)-.055f,.18f,world(b.z)-.055f,.11f,.11f,.11f,1.f,.82f,.20f,0,false,.8f});
        }
        for(const auto& b:game.combat.hostileBullets) {
            out.push_back({world(b.x)-.06f,.28f,world(b.z)-.06f,.12f,.12f,.12f,.9f,.12f,1,0,false,.8f});
        }
        const float px=world(game.player.x),pz=world(game.player.z);
        const float dx=game.combat.directionX,dz=game.combat.directionZ;
        const float yaw=std::atan2(dx,dz);
        out.push_back({px-.19f,.012f,pz-.19f,.38f,.01f,.38f,.025f,.04f,.06f});
        const auto playerColor=game.player.dashRemaining>0?engine::Vec3{1,1,1}:game.effects.hurtFlash>0?engine::Vec3{1,.25f,.3f}:engine::Vec3{.07f,.83f,1};
        out.push_back({px-.14f,0,pz-.14f,.28f,.42f,.28f,playerColor.x,playerColor.y,playerColor.z,yaw});
        out.push_back({px-.11f,.42f,pz-.11f,.22f,.16f,.22f,.12f,.48f,.65f,yaw});
        const float recoil=game.effects.muzzle>0?.05f:0;
        out.push_back({px+dx*(.27f-recoil)-.065f,.30f,pz+dz*(.27f-recoil)-.22f,
                       .13f,.13f,.44f,.6f,.68f,.73f,yaw});
        if(game.effects.muzzle>0) out.push_back({px+dx*.52f-.09f,.29f,pz+dz*.52f-.09f,
                                                .18f,.18f,.18f,1,.93f,.45f,yaw});
        for(const auto& p:game.effects.particles) {
            const float size=p.size*(p.life/p.duration);
            out.push_back({p.position.x-size*.5f,std::max(.02f,p.position.y),p.position.z-size*.5f,
                           size,size,size,p.color.x,p.color.y,p.color.z,0,false,.5f});
        }
        if (game.player.alive() && (game.combat.hasTarget || game.manualAim)) {
            float ax=world(game.combat.aimX), az=world(game.combat.aimZ);
            out.push_back({ax-.31f,.55f,az-.015f,.17f,.025f,.03f,1.f,1.f,1.f});
            out.push_back({ax+.14f,.55f,az-.015f,.17f,.025f,.03f,1.f,1.f,1.f});
            out.push_back({ax-.015f,.55f,az-.31f,.03f,.025f,.17f,1.f,1.f,1.f});
            out.push_back({ax-.015f,.55f,az+.14f,.03f,.025f,.17f,1.f,1.f,1.f});
        }

    }
};
