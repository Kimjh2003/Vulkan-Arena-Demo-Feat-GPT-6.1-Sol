#version 450


layout(location=0) in vec3 vertexPosition;
layout(location=1) in vec3 vertexNormal;
layout(location=2) in vec3 origin;
layout(location=3) in vec3 extent;
layout(location=4) in vec3 color;
layout(location=5) in vec2 rotation;
layout(location=6) in vec2 material;
layout(set=0,binding=0,std140) uniform FrameData { vec4 projection; vec4 camera; };
layout(push_constant) uniform Pass { uint shadowPass; } pass;
layout(location=0) out vec3 worldPosition;
layout(location=1) out vec3 worldNormal;
layout(location=2) out vec3 tint;
layout(location=3) out vec2 uv;
layout(location=4) flat out vec2 surfaceMaterial;
layout(location=5) out vec3 lightPosition;
vec3 lightClip(vec3 p) {
    vec3 light=normalize(vec3(-.45,.85,.35));
    vec3 right=normalize(vec3(light.z,0.,-light.x));
    vec3 up=cross(light,right);
    return vec3(dot(p,right)/7.,dot(p,up)/7.,-dot(p,light)/12.);
}
void main() {
    vec3 p=vertexPosition*extent;
    p.xz-=extent.xz*.5;
    p.xz=vec2(p.x*rotation.x+p.z*rotation.y,-p.x*rotation.y+p.z*rotation.x);
    p.xz+=extent.xz*.5;
    worldPosition=origin+p;
    worldNormal=vec3(vertexNormal.x*rotation.x+vertexNormal.z*rotation.y,
                     vertexNormal.y,-vertexNormal.x*rotation.y+vertexNormal.z*rotation.x);
    vec3 lc=lightClip(worldPosition);
    lightPosition=vec3(lc.x*.5+.5,.5-lc.y*.5,lc.z*.5+.5);
    tint=color; uv=worldPosition.xz*.25; surfaceMaterial=material;
    if(pass.shadowPass!=0u) {gl_Position=vec4(lc.x,-lc.y,lc.z*.5+.5,1.);return;}
    vec3 relative=worldPosition-vec3(0.,7.,7.);
    float viewY=dot(relative,vec3(0.,.70710678,-.70710678));
    float depth=dot(relative,vec3(0.,-.70710678,-.70710678));
    gl_Position=vec4(relative.x*projection.x,viewY*projection.y,
                     depth*projection.z+projection.w,depth);
    gl_Position.xy+=camera.xy*depth;
    gl_Position.y=-gl_Position.y;
    int transform=int(camera.w);
    if(transform>=4) {gl_Position.x=-gl_Position.x;transform-=4;}
    if(transform==1) gl_Position.xy=vec2(-gl_Position.y,gl_Position.x);
    else if(transform==2) gl_Position.xy=-gl_Position.xy;
    else if(transform==3) gl_Position.xy=vec2(gl_Position.y,-gl_Position.x);
}
