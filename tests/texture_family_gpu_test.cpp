#include <EGL/egl.h>
#include "material_texture.h"
#include "scene_pipeline.h"
#include "gl_program.h"
#include <cassert>
#include <cstdio>
#include <cmath>

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
    const auto caps=graphics::queryGlCapabilities();
    GLuint sample=graphics::createProgram({{GL_VERTEX_SHADER,R"(#version 300 es
out vec2 uv;
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));uv=p;gl_Position=vec4(p*2.-1.,0.,1.);})"},
        {GL_FRAGMENT_SHADER,R"(#version 300 es
precision highp float;in vec2 uv;uniform sampler2D image;out vec4 color;
void main(){color=texture(image,uv);})"}});
    GLuint vao=0;glGenVertexArrays(1,&vao);
    graphics::ScenePipeline pipeline;pipeline.initialize("#version 300 es\n");
    for(auto family:{graphics::TextureFamily::Etc1,graphics::TextureFamily::Etc2,graphics::TextureFamily::Astc}) {
        auto texture=graphics::loadFloor(caps,family);assert(texture.id && texture.family==family);
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture.id);
        GLint minFilter=0;glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,&minFilter);
        assert(minFilter==GL_LINEAR_MIPMAP_LINEAR);
        if(caps.anisotropy>1) {
            GLfloat actual=0;glGetTexParameterfv(GL_TEXTURE_2D,0x84FE,&actual);assert(actual==caps.anisotropy);
        }
        glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,64,64);glDisable(GL_DEPTH_TEST);
        glBindVertexArray(vao);glUseProgram(sample);glUniform1i(glGetUniformLocation(sample,"image"),0);
        glDrawArrays(GL_TRIANGLES,0,3);
        unsigned char rgba[4]{};glReadPixels(1,1,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
        assert(std::abs(static_cast<int>(rgba[0])-223)<=2);
        assert(rgba[0]==rgba[1] && rgba[1]==rgba[2] && rgba[3]==255);
        glEnable(GL_DEPTH_TEST);glViewport(0,0,128,128);
        glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        pipeline.begin();pipeline.add({-4,-.18f,-4,8,.18f,8,.1f,.3f,.5f,0,true});
        pipeline.add({-.5f,0,-.5f,1,1,1,.9f,.2f,.1f,.4f});
        pipeline.draw(128,128,texture.id,0,0);assert(glGetError()==GL_NO_ERROR);
        glDeleteTextures(1,&texture.id);
        printf("Codec %d: equivalent decoded color, shared sampling and shadow pipeline passed\n",static_cast<int>(family));
    }
    graphics::GlCapabilities absent;absent.major=2;
    auto fallback=graphics::loadFloor(absent,graphics::TextureFamily::Astc);
    assert(fallback.id && fallback.family==graphics::TextureFamily::Auto);glDeleteTextures(1,&fallback.id);
    pipeline.reset();glDeleteProgram(sample);glDeleteVertexArrays(1,&vao);assert(glGetError()==GL_NO_ERROR);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
    puts("All texture-family checks passed");
}
