#pragma once
#include "gameplay/game_session.h"
#include <cstdio>
#include <string>

struct HudPresenter {
    // Numeric state for the native Android HUD; no parsing of display labels.
    static std::string state(const GameSession& game) {
        char data[512];
        std::snprintf(data, sizeof(data),
            "{\"score\":%d,\"ammo\":%d,\"magazine\":%d,\"hp\":%d,\"seconds\":%d,\"wave\":%d,"
            "\"manual\":%s,\"reloading\":%s,\"ended\":%s,\"victory\":%s}",
            game.score, game.combat.ammo, game.combat.capacity(), game.player.hp,
            std::clamp(int(game.age),0,300), game.wave(), game.manualAim?"true":"false",
            game.combat.reload>0?"true":"false", game.gameOver()?"true":"false", game.victory()?"true":"false");
        return std::string(data).substr(0,std::string(data).size()-1)+
            ",\"weapon\":"+std::to_string(int(game.combat.weapon))+
            ",\"upgrading\":"+(game.upgrading()?"true":"false")+
            ",\"dash\":"+std::to_string(game.player.dashCooldown)+"}";
    }
    static std::string text(const GameSession& game) {
        char text[160];
        if (game.combat.reload > 0) {
            std::snprintf(text, sizeof(text), "AMMO %d/%d · RELOADING   HP %d/5   SCORE %d%s",
                          game.combat.ammo, game.combat.capacity(), game.player.hp, game.score,
                          game.player.hp <= 0 ? "\nGAME OVER — Back to restart" : "");
        } else {
            std::snprintf(text, sizeof(text), "AMMO %d/%d   HP %d/5   SCORE %d%s",
                          game.combat.ammo, game.combat.capacity(), game.player.hp, game.score,
                          game.player.hp <= 0 ? "\nGAME OVER — Back to restart" : "");
        }
        const std::string mode=game.manualAim?"MANUAL":"AUTO";
        return std::string(text)+"\n"+mode+"  WAVE "+std::to_string(game.wave())+"/5"+
               (game.victory()?"  SURVIVED! Back: menu":"");
    }
};
