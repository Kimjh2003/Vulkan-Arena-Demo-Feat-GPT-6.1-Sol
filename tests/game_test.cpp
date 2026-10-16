#include "../app/src/main/cpp/game.h"
#include "../app/src/main/cpp/camera3d.h"
#include "../app/src/main/cpp/scene_builder.h"
#include "../app/src/main/cpp/hud_presenter.h"
#include "../app/src/main/cpp/frame_state.h"
#include <cassert>
#include <iostream>
int main() {
    Game a,b;
    for(int i=0;i<1000;i++) {a.update(.016f,.7f,.3f,true);b.update(.016f,.7f,.3f,true);}
    assert(a.score==b.score && a.player.hp==b.player.hp && a.enemySystem.enemies.size()==b.enemySystem.enemies.size());
    assert(a.player.x>=.02f && a.player.x<=.98f && a.player.z>=.03f && a.player.z<=.97f);
    Game hit;hit.enemySystem.enemies.push_back({hit.player.x,hit.player.z});hit.combat.fire=100;hit.enemySystem.spawn=100;
    hit.update(.01f,0,0,false);assert(hit.player.hp==4 && hit.enemySystem.enemies.empty());
    Game shot;shot.enemySystem.enemies.push_back({.7f,.5f});shot.combat.bullets.push_back({.7f,.5f});shot.combat.fire=100;shot.enemySystem.spawn=100;
    shot.update(0,0,0,false);assert(shot.score==10 && shot.enemySystem.enemies.empty() && shot.combat.bullets.empty());
    Game dead;dead.player.hp=0;dead.update(1,1,1,true);assert(dead.age==0 && dead.player.x==.5f);
    for(auto box:SceneBuilder::build(a)) assert(std::isfinite(box.x) && std::isfinite(box.z) && box.w>0 && box.h>0 && box.d>0);
    std::vector<Box> fixed,moving,joined;SceneBuilder::buildStatic(fixed);SceneBuilder::buildDynamic(a,moving);
    joined=fixed;joined.insert(joined.end(),moving.begin(),moving.end());
    auto original=SceneBuilder::build(a);assert(original.size()==joined.size());
    for(size_t i=0;i<original.size();++i){const auto& x=original[i];const auto& y=joined[i];
        assert(x.x==y.x&&x.y==y.y&&x.z==y.z&&x.w==y.w&&x.h==y.h&&x.d==y.d&&
            x.r==y.r&&x.g==y.g&&x.b==y.b&&x.yaw==y.yaw&&x.textured==y.textured&&x.emissive==y.emissive);}
    a.player.dashCooldown=1.234567f;FrameState wire;wire.capture(a);
    float dashValue=0;std::memcpy(&dashValue,&wire.values[12],4);
    assert(wire.values[0]==a.score&&wire.values[3]==a.player.hp&&dashValue==a.player.dashCooldown);
    auto center=Camera3D::screenToGround(.5f,.5f,16.f/9.f);
    assert(std::abs(center.x-.5f)<.0001f && std::abs(center.z-.5f)<.0001f);
    auto right=Camera3D::screenToGround(.6f,.5f,16.f/9.f);
    assert(right.x>center.x && std::abs(right.z-center.z)<.0001f);
    auto bottom=Camera3D::screenToGround(.5f,.6f,16.f/9.f);
    assert(bottom.z>center.z);
    Game magazine;magazine.enemySystem.spawn=100;magazine.enemySystem.enemies.push_back({.9f,.5f});
    magazine.update(0,0,0,false);
    assert(magazine.combat.ammo==11 && magazine.combat.hasTarget && magazine.combat.aimX==.9f);
    magazine.combat.ammo=1;magazine.combat.fire=0;
    magazine.update(0,0,0,false);assert(magazine.combat.ammo==0 && magazine.combat.reload>0);
    auto count=magazine.combat.bullets.size();
    magazine.update(0,0,0,false);assert(magazine.combat.bullets.size()==count);
    magazine.enemySystem.enemies.clear();
    for(int i=0;i<30;i++) magazine.update(.05f,0,0,false);
    assert(magazine.combat.ammo==CombatSystem::magazineSize && magazine.combat.reload==0);
    Game idle;idle.enemySystem.spawn=100;idle.update(.01f,0,0,false);
    assert(idle.combat.ammo==CombatSystem::magazineSize && !idle.combat.hasTarget);
    assert(HudPresenter::text(idle).find("AMMO 12/12") != std::string::npos);
    assert(HudPresenter::text(dead).find("GAME OVER") != std::string::npos);
    // Actual gameplay: both modes share collision, ammunition and damage rules.
    Game manual;manual.manualAim=true;manual.enemySystem.spawn=100;
    manual.enemySystem.enemies.emplace_back(.8f,.5f);
    manual.updateControls(0,0,0,1,0,false);
    assert(manual.combat.bullets.empty() && manual.combat.ammo==12);
    manual.updateControls(0,0,0,1,0,true);
    assert(manual.combat.ammo==11 && manual.combat.bullets.size()==1);
    assert(manual.combat.bullets.front().vx>0 && manual.combat.bullets.front().vz==0);
    assert(manual.effects.events & engine::Shot);
    manual.updateControls(.01f,0,0,0,0,false);
    assert(!(manual.effects.events & engine::Shot));

    float wallX=.6f,wallZ=.33f;
    World::move(wallX,wallZ,.35f,0);
    assert(wallX<.66f && !World::blocked(wallX,wallZ));
    assert(!World::clearShot(.6f,.33f,.9f,.33f));
    assert(World::clearShot(.5f,.5f,.9f,.5f));
    Game covered;covered.enemySystem.spawn=100;covered.combat.fire=100;
    covered.enemySystem.enemies.emplace_back(.9f,.33f);
    covered.combat.bullets.push_back({.6f,.33f,10,0});
    covered.update(.05f,0,0,false);
    assert(covered.score==0 && covered.enemySystem.enemies.size()==1 && covered.combat.bullets.empty());
    assert(covered.effects.events & engine::Impact);
    assert(!covered.effects.particles.empty());

    Game swept;swept.enemySystem.spawn=100;swept.combat.fire=100;
    swept.enemySystem.enemies.emplace_back(.7f,.5f);
    swept.combat.bullets.push_back({.5f,.5f,8,0});
    swept.update(.05f,0,0,false);
    assert(swept.score==10 && swept.enemySystem.enemies.empty());

    Game ranged;ranged.enemySystem.spawn=100;ranged.combat.fire=100;
    ranged.enemySystem.enemies.emplace_back(.8f,.5f,EnemyKind::Shooter);
    ranged.enemySystem.enemies.front().cooldown=0;
    ranged.update(0,0,0,false);
    assert(ranged.enemySystem.enemies.front().windup>0 && ranged.combat.hostileBullets.empty());
    ranged.enemySystem.chase(.45f,ranged.player);
    assert(ranged.enemySystem.shots.size()==1 && ranged.enemySystem.shots.front().vx<0);
    ranged.combat.hostileBullets={Body{.6f,.5f,-4,0}};
    ranged.update(.05f,0,0,false);
    assert(ranged.player.hp==4 && (ranged.effects.events & engine::Hurt));

    EnemySystem chargers;Player stationary;
    chargers.enemies.emplace_back(.8f,.5f,EnemyKind::Charger);
    chargers.enemies.front().cooldown=0;
    chargers.chase(0,stationary);
    assert(chargers.enemies.front().windup>0 && chargers.enemies.front().x==.8f);
    chargers.chase(.6f,stationary);chargers.chase(.1f,stationary);
    assert(chargers.enemies.front().charging>0 && chargers.enemies.front().x<.8f);

    Game tough;tough.enemySystem.spawn=100;tough.combat.fire=100;
    tough.enemySystem.enemies.emplace_back(.7f,.5f,EnemyKind::Shooter);
    tough.combat.bullets.push_back({.7f,.5f});tough.update(0,0,0,false);
    assert(tough.score==0 && tough.enemySystem.enemies.front().hp==1);
    tough.combat.bullets.push_back({.7f,.5f});tough.update(0,0,0,false);
    assert(tough.score==10 && tough.enemySystem.enemies.empty());

    EnemySystem routing;Player behindCover;behindCover.x=.6f;behindCover.z=.33f;
    routing.enemies.emplace_back(.9f,.33f);
    for(int i=0;i<600;++i) routing.chase(.016f,behindCover);
    assert(std::hypot(routing.enemies.front().x-behindCover.x,
                      routing.enemies.front().z-behindCover.z)<.06f);
    assert(!World::blocked(routing.enemies.front().x,routing.enemies.front().z,.017f));
    Player protectedPlayer;
    assert(protectedPlayer.damage() && !protectedPlayer.damage() && protectedPlayer.hp==4);
    protectedPlayer.tick(.5f);assert(protectedPlayer.damage() && protectedPlayer.hp==3);

    Game won;won.age=300;won.updateControls(.05f,1,0,1,0,true);
    assert(won.victory() && won.player.x==.5f && won.enemySystem.enemies.empty());
    assert(HudPresenter::text(won).find("SURVIVED")!=std::string::npos);
    assert(HudPresenter::state(won).find("\"victory\":true")!=std::string::npos);
    assert(HudPresenter::state(won).find("\"seconds\":300")!=std::string::npos);
    assert(HudPresenter::state(dead).find("\"ended\":true")!=std::string::npos);
    // Weapon identities, magazine limits, pellet spread and actual upgraded damage.
    for(int kind=0;kind<3;++kind) {
        Game weapon;weapon.manualAim=true;weapon.enemySystem.spawn=100;
        weapon.combat.equip(kind);int capacity=weapon.combat.capacity();
        weapon.updateControls(0,0,0,1,0,true);
        assert(weapon.combat.ammo==capacity-1);
        assert(weapon.combat.bullets.size()==size_t(kind==1?5:1));
        if(kind==1) assert(weapon.combat.bullets.front().vz<0 && weapon.combat.bullets.back().vz>0);
    }
    Player dasher;assert(dasher.dash(1,0));assert(!dasher.dash(1,0));
    assert(!dasher.damage());dasher.moveStick(.1f,0,0);assert(dasher.x>.6f);
    dasher.tick(2.6f);assert(dasher.dash(0,-1));
    Player wallDash;wallDash.x=.6f;wallDash.z=.33f;wallDash.dash(1,0);wallDash.moveStick(.18f,0,0);
    assert(wallDash.x<.66f && !World::blocked(wallDash.x,wallDash.z));
    Game next;next.age=59.99f;next.enemySystem.spawn=100;next.updateControls(.05f,0,0,0,0,false);
    assert(next.upgrading() && next.wave()==2);float frozen=next.age;
    next.updateControls(.05f,1,0,1,0,true);assert(next.age==frozen && next.combat.bullets.empty());
    assert(!next.chooseUpgrade(0) && next.upgrading());assert(next.chooseUpgrade(1));
    assert(next.combat.damageBonus==1 && !next.chooseUpgrade(1));
    next.updateControls(.01f,0,0,0,0,false);assert(!next.upgrading() && next.age>frozen);
    next.age=120;next.checkWave();assert(next.chooseUpgrade(2) && next.combat.rateLevel==1);
    next.age=180;next.checkWave();assert(next.chooseUpgrade(3) && next.player.speedBonus>1);
    next.enemySystem.spawn=100;next.enemySystem.enemies.emplace_back(.7f,.5f,EnemyKind::Shooter);
    next.combat.fire=100;next.combat.bullets.push_back({.7f,.5f});next.updateControls(0,0,0,0,0,false);
    assert(next.score==10);
    assert(HudPresenter::state(next).find("\"upgrading\":false")!=std::string::npos);
    std::cout << "Game logic checks passed\n";
}
