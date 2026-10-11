#include "scene_pipeline.h"
#include "gl_program.h"
#include "camera3d.h"
#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace graphics {
void ScenePipeline::initialize(const std::string& version) {
    const std::string vertex=version+R"(
precision highp float;
layout(location=0) in vec3 vertexPosition;
layout(location=1) in vec3 vertexNormal;
layout(location=2) in vec3 origin;
layout(location=3) in vec3 extent;
layout(location=4) in vec3 color;
layout(location=5) in vec2 rotation;
layout(location=6) in vec2 material;
layout(std140) uniform FrameData { vec4 projection; vec4 camera; };
uniform bool shadowPass;
out highp vec3 worldPosition;
out highp vec3 worldNormal;
out vec3 tint;
out vec2 uv;
flat out vec2 surfaceMaterial;
out highp vec3 lightPosition;
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
    lightPosition=lightClip(worldPosition)*.5+.5;
    tint=color; uv=worldPosition.xz*.25; surfaceMaterial=material;
    if(shadowPass) {gl_Position=vec4(lightClip(worldPosition),1.);return;}
    vec3 relative=worldPosition-vec3(0.,7.,7.);
    float viewY=dot(relative,vec3(0.,.70710678,-.70710678));
    float depth=dot(relative,vec3(0.,-.70710678,-.70710678));
    gl_Position=vec4(relative.x*projection.x,viewY*projection.y,
                     depth*projection.z+projection.w,depth);
    gl_Position.xy+=camera.xy*depth;
}
)";
    const std::string fragment=version+R"(
precision highp float;
in highp vec3 worldPosition;
in highp vec3 worldNormal;
in vec3 tint;
in vec2 uv;
flat in vec2 surfaceMaterial;
in highp vec3 lightPosition;
uniform sampler2D floorMap;
uniform highp sampler2DShadow shadowMap;
uniform bool shadowPass;
uniform bool shadowsEnabled;
out vec4 fragment;
float visibility(vec3 n,vec3 l) {
    if(!shadowsEnabled || any(lessThan(lightPosition,vec3(0.))) ||
       any(greaterThan(lightPosition,vec3(1.)))) return 1.;
    float bias=max(.0012,.003*(1.-max(dot(n,l),0.)));
    float value=0.;
    vec2 texel=1./vec2(textureSize(shadowMap,0));
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x)
        value+=texture(shadowMap,vec3(lightPosition.xy+vec2(float(x),float(y))*texel,
                                    lightPosition.z-bias));
    return value/9.;
}
void main() {
    if(shadowPass) {fragment=vec4(1.);return;}
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
)";
    program=createProgram({{GL_VERTEX_SHADER,vertex.c_str()},{GL_FRAGMENT_SHADER,fragment.c_str()}});
    glUseProgram(program);
    passLocation=glGetUniformLocation(program,"shadowPass");
    shadowLocation=glGetUniformLocation(program,"shadowsEnabled");
    glUniform1i(glGetUniformLocation(program,"floorMap"),0);
    glUniform1i(glGetUniformLocation(program,"shadowMap"),1);
    const GLuint block=glGetUniformBlockIndex(program,"FrameData");
    if(block==GL_INVALID_INDEX) throw std::runtime_error("Frame UBO unavailable");
    glUniformBlockBinding(program,block,0);
    glGenBuffers(1,&uniforms);glBindBuffer(GL_UNIFORM_BUFFER,uniforms);
    glBufferData(GL_UNIFORM_BUFFER,8*sizeof(float),nullptr,GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER,0,uniforms);

    struct Vertex {float p[3],n[3];};
    std::vector<Vertex> cube;
    auto face=[&](float nx,float ny,float nz,std::initializer_list<float> points) {
        auto p=points.begin(); Vertex corners[4]{};
        for(auto& corner:corners) {
            for(float& coordinate:corner.p) coordinate=*p++;
            corner.n[0]=nx;corner.n[1]=ny;corner.n[2]=nz;
        }
        for(int i:{0,1,2,2,1,3}) cube.push_back(corners[i]);
    };
    face(0,1,0,{0,1,0,0,1,1,1,1,0,1,1,1});
    face(0,-1,0,{0,0,1,0,0,0,1,0,1,1,0,0});
    face(0,0,1,{0,0,1,1,0,1,0,1,1,1,1,1});
    face(0,0,-1,{1,0,0,0,0,0,1,1,0,0,1,0});
    face(1,0,0,{1,0,1,1,0,0,1,1,1,1,1,0});
    face(-1,0,0,{0,0,0,0,0,1,0,1,0,0,1,1});
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);
    glGenBuffers(1,&mesh);glBindBuffer(GL_ARRAY_BUFFER,mesh);
    glBufferData(GL_ARRAY_BUFFER,cube.size()*sizeof(Vertex),cube.data(),GL_STATIC_DRAW);
    for(GLuint i=0;i<2;++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(i*3*sizeof(float)));
    }
    glGenBuffers(1,&stream);glBindBuffer(GL_ARRAY_BUFFER,stream);
    const size_t offsets[]={offsetof(Instance,origin),offsetof(Instance,extent),offsetof(Instance,color),
                            offsetof(Instance,rotation),offsetof(Instance,material)};
    for(GLuint i=0;i<5;++i) {
        glEnableVertexAttribArray(i+2);
        glVertexAttribPointer(i+2,i<3?3:2,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>(offsets[i]));
        glVertexAttribDivisor(i+2,1);
    }
    GLint previous=0;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&previous);
    glGenTextures(1,&shadowTexture);glBindTexture(GL_TEXTURE_2D,shadowTexture);
    glTexStorage2D(GL_TEXTURE_2D,1,GL_DEPTH_COMPONENT24,shadowSize,shadowSize);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_FUNC,GL_LEQUAL);
    glGenFramebuffers(1,&shadowFramebuffer);glBindFramebuffer(GL_FRAMEBUFFER,shadowFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,shadowTexture,0);
    const GLenum none=GL_NONE;glDrawBuffers(1,&none);glReadBuffer(GL_NONE);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) {
        glDeleteFramebuffers(1,&shadowFramebuffer);shadowFramebuffer=0;
        glDeleteTextures(1,&shadowTexture);shadowTexture=0;
    }
    glBindFramebuffer(GL_FRAMEBUFFER,previous);glBindTexture(GL_TEXTURE_2D,0);
    __android_log_print(ANDROID_LOG_INFO,"Arena","Scene pipeline: GPU instancing + UBO + per-pixel lighting; shadow=%s",
                       shadowFramebuffer?"1024 PCF":"disabled (FBO fallback)");
}
void ScenePipeline::add(const Box& box) {
    if(box.w<=0 || box.h<=0 || box.d<=0) return;
    instances.push_back({{box.x,box.y,box.z},{box.w,box.h,box.d},{box.r,box.g,box.b},
                         {std::cos(box.yaw),std::sin(box.yaw)},{box.textured?1.f:0.f,box.emissive}});
}
void ScenePipeline::draw(int width,int height,GLuint floorTexture,float shake,float phase) {
    GLint target=0;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&target);
    const float aspect=static_cast<float>(width)/std::max(1,height);
    const float scale=1.f/Camera3D::tanHalfFov;
    const float depth=(Camera3D::farPlane+Camera3D::nearPlane)/(Camera3D::farPlane-Camera3D::nearPlane);
    const float offset=-2.f*Camera3D::farPlane*Camera3D::nearPlane/(Camera3D::farPlane-Camera3D::nearPlane);
    const float frame[]={scale/aspect,scale,depth,offset,std::sin(phase*2.4f)*shake,std::cos(phase*1.7f)*shake,0,0};
    glBindBuffer(GL_UNIFORM_BUFFER,uniforms);glBufferSubData(GL_UNIFORM_BUFFER,0,sizeof(frame),frame);
    glBindBufferBase(GL_UNIFORM_BUFFER,0,uniforms);
    const auto firstNonCaster=std::partition(instances.begin(),instances.end(),[](const Instance& i) {
        return i.extent[1]>.08f && i.origin[1]>=0 && i.material[1]<.1f;
    });
    const GLsizei casters=static_cast<GLsizei>(firstNonCaster-instances.begin());
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,stream);
    if(!instances.empty()) {
        const size_t bytes=instances.size()*sizeof(Instance);
        capacity=std::max({capacity,bytes,size_t(128*sizeof(Instance))});
        glBufferData(GL_ARRAY_BUFFER,capacity,nullptr,GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER,0,bytes,instances.data());
    }
    glUseProgram(program);
    glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,0);
    if(shadowFramebuffer && casters) {
        glBindFramebuffer(GL_FRAMEBUFFER,shadowFramebuffer);glViewport(0,0,shadowSize,shadowSize);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(2.f,4.f);
        glUniform1i(passLocation,1);
        glDrawArraysInstanced(GL_TRIANGLES,0,36,casters);
        glDisable(GL_POLYGON_OFFSET_FILL);
    } else if(shadowFramebuffer) {
        glBindFramebuffer(GL_FRAMEBUFFER,shadowFramebuffer);glClear(GL_DEPTH_BUFFER_BIT);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,target);glViewport(0,0,width,height);
    glUniform1i(passLocation,0);glUniform1i(shadowLocation,shadowFramebuffer?1:0);
    glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,shadowTexture);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,floorTexture);
    // Failed ETC upload: disable the floor material, preserving solid-color rendering.
    // A uniform fallback is handled through a white 1x1 texture supplied by the renderer.
    if(!instances.empty()) glDrawArraysInstanced(GL_TRIANGLES,0,36,static_cast<GLsizei>(instances.size()));
}
void ScenePipeline::reset() noexcept {
    if(shadowFramebuffer) glDeleteFramebuffers(1,&shadowFramebuffer);
    if(shadowTexture) glDeleteTextures(1,&shadowTexture);
    if(program) glDeleteProgram(program);
    if(vao) glDeleteVertexArrays(1,&vao);
    for(GLuint id:{mesh,stream,uniforms}) if(id) glDeleteBuffers(1,&id);
    shadowFramebuffer=shadowTexture=program=vao=mesh=stream=uniforms=0;
    capacity=0;instances.clear();
}
}
