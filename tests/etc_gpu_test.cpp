#include <EGL/egl.h>
#include "etc_gles.h"
#include <cassert>
#include <cstdio>

int main() {
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(eglInitialize(display,nullptr,nullptr)); assert(eglBindAPI(EGL_OPENGL_ES_API));
    const EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
                         EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};
    EGLConfig config; EGLint count=0;
    assert(eglChooseConfig(display,attrs,&config,1,&count) && count);
    const EGLint surfaceAttrs[]={EGL_WIDTH,8,EGL_HEIGHT,8,EGL_NONE};
    EGLSurface surface=eglCreatePbufferSurface(display,config,surfaceAttrs);
    const EGLint contextAttrs[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,contextAttrs);
    assert(surface!=EGL_NO_SURFACE && context!=EGL_NO_CONTEXT);
    assert(eglMakeCurrent(display,surface,surface,context));
    const auto caps=etc::queryCapabilities(); assert(caps.etc2);
    GLuint prior=0; glGenTextures(1,&prior); glBindTexture(GL_TEXTURE_2D,prior);
    for(int i=0;i<=static_cast<int>(etc::Format::EacSignedRg);++i) {
        const auto format=static_cast<etc::Format>(i);
        if(format==etc::Format::Etc1Rgb && !caps.etc1) {puts("ETC1 extension absent: skipped");continue;}
        std::vector<graphics::CompressedMip> mips;
        for(int extent:{4,2,1}) mips.push_back({extent,extent,std::vector<uint8_t>(etc::blockBytes(format),0)});
        GLuint texture=etc::upload(caps,format,mips); assert(texture && glIsTexture(texture));
        GLint binding=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding);assert(static_cast<GLuint>(binding)==prior);
        assert(glGetError()==GL_NO_ERROR);glDeleteTextures(1,&texture);
        printf("%s: upload passed\n",etc::name(format));
        auto bad=mips;bad[0].bytes.pop_back();assert(!etc::upload(caps,format,bad));
        bad=mips;bad.pop_back();assert(!etc::upload(caps,format,bad));
        bad=mips;bad[1].width=3;assert(!etc::upload(caps,format,bad));
        assert(!etc::upload({},format,mips));
    }
    assert(!etc::upload(caps,static_cast<etc::Format>(999),{}));
    GLuint floor=etc::createFloorTexture(caps);assert(floor);glDeleteTextures(1,&floor);
    glDeleteTextures(1,&prior);assert(glGetError()==GL_NO_ERROR);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
    puts("ETC upload and validation checks passed");
}
