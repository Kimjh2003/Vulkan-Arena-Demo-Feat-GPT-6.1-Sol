#pragma once
#include "collision.h"
#include <vector>

namespace engine {
enum Event : unsigned { Shot=1, Impact=2, Hurt=4 };
struct Particle {
    Vec3 position, velocity;
    float life, duration, size;
    Vec3 color;
};
struct Effects {
    std::vector<Particle> particles;
    unsigned events=0;
    float shake=0, hurtFlash=0, muzzle=0;
    void beginFrame(float dt) {
        events=0;
        shake=std::max(0.f,shake-dt*.07f);
        hurtFlash=std::max(0.f,hurtFlash-dt);
        muzzle=std::max(0.f,muzzle-dt);
        for(auto& p:particles) {
            p.life-=dt;
            p.position.x+=p.velocity.x*dt;p.position.y+=p.velocity.y*dt;p.position.z+=p.velocity.z*dt;
            p.velocity.y-=4.f*dt;
        }
        particles.erase(std::remove_if(particles.begin(),particles.end(),
                        [](const Particle& p){return p.life<=0;}),particles.end());
    }
    void burst(float x,float z,Vec3 color,int count) {
        for(int i=0;i<count && particles.size()<160;++i) {
            const float angle=i*2.399963f;
            particles.push_back({{(x-.5f)*8.f,.3f,(z-.5f)*8.f},
                                {std::cos(angle)*1.6f,1.4f+float(i%3)*.3f,std::sin(angle)*1.6f},
                                .24f,.24f,.07f,color});
        }
    }
    void shot() {events|=Shot;muzzle=.045f;shake=std::max(shake,.0025f);}
    void impact(float x,float z,bool flesh) {
        events|=Impact;burst(x,z,flesh?Vec3{1,.3f,.15f}:Vec3{1,.8f,.4f},flesh?8:4);
    }
    void hurt(float x,float z) {events|=Hurt;hurtFlash=.25f;shake=.012f;burst(x,z,{1,.1f,.1f},10);}
};
} // namespace engine
