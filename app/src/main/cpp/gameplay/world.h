#pragma once
#include <algorithm>
#include <cmath>
#include <array>
#include "../engine/collision.h"

struct Body { float x, z, vx = 0, vz = 0; };

// Normalized simulation space. No Android or graphics dependencies.
struct World {
    static constexpr float extent = 8.f;
    static float toWorld(float value) { return (value - .5f) * extent; }
    static float toGround(float value) { return value / extent + .5f; }
    static constexpr std::array<engine::Aabb,6> cover = {{
        {{-2.6f,0,-1.6f},{-1.4f,.8f,-1.1f}},
        {{1.4f,0,-1.6f},{2.6f,.8f,-1.1f}},
        {{-2.6f,0,1.1f},{-1.4f,.8f,1.6f}},
        {{1.4f,0,1.1f},{2.6f,.8f,1.6f}},
        {{-.6f,0,-2.8f},{.6f,1.f,-2.3f}},
        {{-.6f,0,2.3f},{.6f,1.f,2.8f}}
    }};
    static bool blocked(float x,float z,float radius=.02f) {
        for(const auto& box:cover)
            if(engine::circleBoxXZ({toWorld(x),0,toWorld(z)},radius*extent,box)) return true;
        return false;
    }
    static std::optional<float> firstHit(float x,float z,float endX,float endZ,float radius=0) {
        std::optional<float> first;
        for(auto box:cover) {
            box.min.x-=radius*extent;box.min.z-=radius*extent;
            box.max.x+=radius*extent;box.max.z+=radius*extent;
            auto hit=engine::segmentBox({toWorld(x),.3f,toWorld(z)},
                                       {toWorld(endX),.3f,toWorld(endZ)},box);
            if(hit && (!first || *hit<*first)) first=hit;
        }
        return first;
    }
    static bool clearShot(float x,float z,float endX,float endZ) {return !firstHit(x,z,endX,endZ);}
    static void move(float& x,float& z,float dx,float dz,float radius=.02f) {
        // Small substeps prevent characters tunnelling through thin cover.
        const int steps=std::max(1,int(std::ceil(std::hypot(dx,dz)/.008f)));
        for(int i=0;i<steps;++i) {
            const float nx=std::clamp(x+dx/steps,radius,1-radius);
            if(!blocked(nx,z,radius)) x=nx;
            const float nz=std::clamp(z+dz/steps,radius,1-radius);
            if(!blocked(x,nz,radius)) z=nz;
        }
    }
    static std::array<float,2> waypoint(float x,float z,float tx,float tz) {
        if(!firstHit(x,z,tx,tz,.021f)) return {tx,tz};
        float best=100;
        std::array<float,2> target{x,z};
        for(const auto& box:cover) {
            auto expanded=box;
            expanded.min.x-=.021f*extent;expanded.min.z-=.021f*extent;
            expanded.max.x+=.021f*extent;expanded.max.z+=.021f*extent;
            if(!engine::segmentBox({toWorld(x),.3f,toWorld(z)},
                {toWorld(tx),.3f,toWorld(tz)},expanded)) continue;
            for(float cx:{toGround(box.min.x)-.035f,toGround(box.max.x)+.035f})
                for(float cz:{toGround(box.min.z)-.035f,toGround(box.max.z)+.035f}) {
                    if(firstHit(x,z,cx,cz,.021f)) continue;
                    const float cost=std::hypot(cx-x,cz-z)+std::hypot(tx-cx,tz-cz)+
                                     (firstHit(cx,cz,tx,tz,.021f)? .5f:0.f);
                    if(cost<best) {best=cost;target={cx,cz};}
                }
        }
        return target;
    }
    static void constrainPlayer(float& x, float& z) {
        x = std::clamp(x, .02f, .98f);
        z = std::clamp(z, .03f, .97f);
    }
    static bool overlaps(float ax, float az, float bx, float bz, float radius) {
        return std::hypot(ax - bx, az - bz) < radius;
    }
    static bool outside(const Body& body) {
        return body.x < 0 || body.x > 1 || body.z < 0 || body.z > 1;
    }
};
