#version 450


layout(location=0) in vec3 worldPosition;
layout(location=1) in vec3 worldNormal;
layout(location=2) in vec3 tint;
layout(location=3) in vec2 uv;
layout(location=4) flat in vec2 surfaceMaterial;
layout(location=5) in vec3 lightPosition;
layout(set=0,binding=1) uniform sampler2D floorMap;
layout(set=0,binding=2) uniform sampler2D shadowMap;

layout(set=0,binding=0,std140) uniform FrameData { vec4 projection; vec4 camera; };
layout(location=0) out vec4 fragment;
float visibility(vec3 n,vec3 l) {
    if(camera.z<.5 || any(lessThan(lightPosition,vec3(0.))) ||
       any(greaterThan(lightPosition,vec3(1.)))) return 1.;
    float bias=max(.0012,.003*(1.-max(dot(n,l),0.)));
    float value=0.;
    vec2 texel=1./vec2(textureSize(shadowMap,0));
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x)
        value+=step(lightPosition.z-bias,texture(shadowMap,
                       lightPosition.xy+vec2(float(x),float(y))*texel).r);
    return value/9.;
}
void main() {

    vec3 n=normalize(worldNormal);
    vec3 l=normalize(vec3(-.45,.85,.35));
    vec3 view=normalize(vec3(0.,7.,7.)-worldPosition);
    float shadow=visibility(n,l);
    float diffuse=max(dot(n,l),0.);
    float specular=pow(max(dot(n,normalize(l+view)),0.),32.)*shadow*.22;
    float rim=pow(1.-max(dot(n,view),0.),3.)*.12;
    vec3 base=tint*(surfaceMaterial.x>.5?texture(floorMap,uv).rgb:vec3(1.));
    vec3 shaded=base*(.32+.68*diffuse*shadow)+vec3(.65,.82,1.)*(specular+rim);
    shaded+=base*surfaceMaterial.y*1.3;
    float fog=smoothstep(10.,19.,length(vec3(0.,7.,7.)-worldPosition))*.32;
    shaded=mix(shaded,vec3(.025,.035,.065),fog);
    fragment=vec4(shaded,1.);
}
