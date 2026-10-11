#pragma once
#include <algorithm>
#include <cmath>
#include <optional>

namespace engine {
struct Vec3 { float x=0, y=0, z=0; };
struct Aabb { Vec3 min, max; };

// Swept segment versus box: returns earliest fraction in [0,1].
inline std::optional<float> segmentBox(Vec3 start, Vec3 end, const Aabb& box) {
    float enter=0, leave=1;
    const float a[]={start.x,start.y,start.z}, b[]={end.x,end.y,end.z};
    const float lo[]={box.min.x,box.min.y,box.min.z}, hi[]={box.max.x,box.max.y,box.max.z};
    for (int axis=0;axis<3;++axis) {
        const float d=b[axis]-a[axis];
        if (std::abs(d)<1e-7f) { if(a[axis]<lo[axis] || a[axis]>hi[axis]) return {}; }
        else {
            float t0=(lo[axis]-a[axis])/d,t1=(hi[axis]-a[axis])/d;
            if(t0>t1) std::swap(t0,t1);
            enter=std::max(enter,t0);leave=std::min(leave,t1);
            if(enter>leave) return {};
        }
    }
    return enter;
}
inline bool circleBoxXZ(Vec3 center,float radius,const Aabb& box) {
    const float dx=center.x-std::clamp(center.x,box.min.x,box.max.x);
    const float dz=center.z-std::clamp(center.z,box.min.z,box.max.z);
    return dx*dx+dz*dz < radius*radius;
}
inline std::optional<float> segmentCircle(float ax,float az,float bx,float bz,
                                        float cx,float cz,float radius) {
    const float dx=bx-ax,dz=bz-az,ox=ax-cx,oz=az-cz;
    const float c=ox*ox+oz*oz-radius*radius;
    if(c<=0) return 0.f;
    const float a=dx*dx+dz*dz;
    if(a<1e-10f) return {};
    const float b=ox*dx+oz*dz,disc=b*b-a*c;
    if(disc<0) return {};
    const float t=(-b-std::sqrt(disc))/a;
    return t>=0 && t<=1 ? std::optional<float>(t) : std::nullopt;
}
} // namespace engine
