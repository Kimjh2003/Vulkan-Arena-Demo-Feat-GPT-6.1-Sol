#include <EGL/egl.h>
#include "scene_pipeline.h"
#include <cassert>
#include <cstdio>
#include <vector>

int main() {
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(eglInitialize(display,nullptr,nullptr));assert(eglBindAPI(EGL_OPENGL_ES_API));
    EGLConfig config;EGLint count=0;
    const EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
                         EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};
    assert(eglChooseConfig(display,attrs,&config,1,&count) && count);
    const EGLint extent[]={EGL_WIDTH,128,EGL_HEIGHT,128,EGL_NONE};
    const EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    EGLSurface surface=eglCreatePbufferSurface(display,config,extent);
    EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);
    assert(eglMakeCurrent(display,surface,surface,context));
    GLuint white=0;glGenTextures(1,&white);glBindTexture(GL_TEXTURE_2D,white);
    const unsigned char pixel[]={255,255,255,255};
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    // Verify the shared GLSL 300 path as well as 320 on this GLES 3.2 driver.
    for(const char* shaderVersion:{"#version 300 es\n","#version 320 es\n"}) {
        graphics::ScenePipeline pipeline;pipeline.initialize(shaderVersion);
        glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
        for(int width:{128,96}) {
            glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,width,128);
            glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            pipeline.begin();
            pipeline.add({-4,-.18f,-4,8,.18f,8,.1f,.3f,.5f,0,true});
            pipeline.add({-.5f,0,-.5f,1,1,1,.9f,.2f,.1f,.4f});
            pipeline.add({1,0,1,.2f,.2f,.2f,1,.8f,.1f,0,false,.8f});
            pipeline.draw(width,128,white,0,0);
            assert(glGetError()==GL_NO_ERROR);
            std::vector<unsigned char> rgba(width*128*4);
            glReadPixels(0,0,width,128,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
            int colored=0;for(size_t i=0;i<rgba.size();i+=4) if(rgba[i] || rgba[i+1] || rgba[i+2]) ++colored;
            assert(colored>200);assert(glGetError()==GL_NO_ERROR);
        }
        pipeline.reset();assert(glGetError()==GL_NO_ERROR);
        printf("%sInstancing/UBO/shadow rendering, resize and cleanup passed\n",shaderVersion);
    }
    glDeleteTextures(1,&white);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
}
