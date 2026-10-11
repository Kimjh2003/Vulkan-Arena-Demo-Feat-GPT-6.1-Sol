#pragma once
#include <algorithm>
enum class WeaponKind { Pistol, Shotgun, Smg };
struct WeaponSpec { int magazine, pellets, damage; float interval, reload, speed, spread; };
inline WeaponSpec weaponSpec(WeaponKind kind) {
    switch(kind) {
        case WeaponKind::Shotgun:return {6,5,1,.65f,1.6f,.85f,.12f};
        case WeaponKind::Smg:return {30,1,1,.085f,1.45f,1.25f,.035f};
        default:return {12,1,1,.22f,1.2f,1.1f,0};
    }
}
