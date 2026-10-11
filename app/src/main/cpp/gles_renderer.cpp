#include "gles_renderer.h"
#include "material_texture.h"
#include <algorithm>
#include <cmath>
#include <android/log.h>
#include <cstddef>
#include <stdexcept>

namespace {
void requireEgl(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(std::string(message) + " (EGL " + std::to_string(eglGetError()) + ")");
    }
}

std::string glText(GLenum name) {
    const auto* value = glGetString(name);
    return value ? reinterpret_cast<const char*>(value) : "unknown";
}
} // namespace

GlesRenderer::GlesRenderer(ANativeWindow* input, graphics::OutputPath path, int preference) : window(input), outputPath(path), texturePreference(preference) {
    if (!window) {
        throw std::runtime_error("Android surface window unavailable");
    }
    ANativeWindow_acquire(window);
    try {
        initialize();
    } catch (...) {
        release();
        throw;
    }
}

GlesRenderer::~GlesRenderer() {
    release();
}

void GlesRenderer::release() noexcept {
    if (display != EGL_NO_DISPLAY) {
        if (context != EGL_NO_CONTEXT && eglGetCurrentContext() == context) {
            extensions.reset();
            if(floorTexture) {glDeleteTextures(1,&floorTexture);floorTexture=0;}
            frameOutput.reset();
            pipeline.reset();
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }
        if (context != EGL_NO_CONTEXT) { eglDestroyContext(display, context); context = EGL_NO_CONTEXT; }
        if (surface != EGL_NO_SURFACE) { eglDestroySurface(display, surface); surface = EGL_NO_SURFACE; }
        eglTerminate(display);
        display = EGL_NO_DISPLAY;
    }
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
}

void GlesRenderer::initialize() {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    requireEgl(display != EGL_NO_DISPLAY, "EGL get display failed");
    requireEgl(eglInitialize(display, nullptr, nullptr), "EGL initialize failed");
    requireEgl(eglBindAPI(EGL_OPENGL_ES_API), "EGL bind OpenGL ES API failed");

    EGLint attrs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_SAMPLE_BUFFERS, 1,
        EGL_SAMPLES, 4,
        EGL_NONE
    };
    EGLConfig config;
    EGLint count = 0;
    // Prefer window MSAA; no extra scene textures or full-screen copy pass.
    for (EGLint samples : {4, 2, 0}) {
        attrs[13] = samples ? 1 : 0;
        attrs[15] = samples;
        requireEgl(eglChooseConfig(display, attrs, &config, 1, &count), "EGL choose config failed");
        if (count) break;
    }
    if (!count) {
        throw std::runtime_error("OpenGL ES 3.0+ config unavailable");
    }

    EGLint visual = 0;
    requireEgl(eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &visual), "EGL get visual failed");
    if (ANativeWindow_setBuffersGeometry(window, 0, 0, visual) != 0) {
        throw std::runtime_error("ANativeWindow set buffers geometry failed");
    }

    for (EGLint minor : {2, 1, 0}) {
        const EGLint ctxAttrs[] = {EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
                                  EGL_CONTEXT_MINOR_VERSION_KHR, minor, EGL_NONE};
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttrs);
        if (context != EGL_NO_CONTEXT) break;
        const EGLint error = eglGetError();
        __android_log_print(ANDROID_LOG_INFO, "Arena", "ES 3.%d context unavailable (EGL 0x%x)", minor, error);
        // Retry version/attribute rejection, but preserve unrelated failures.
        if (error != EGL_BAD_MATCH && error != EGL_BAD_ATTRIBUTE && error != EGL_BAD_CONFIG)
            throw std::runtime_error("EGL context creation failed: " + std::to_string(error));
    }
    if (context == EGL_NO_CONTEXT) {
        const EGLint legacyAttrs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, legacyAttrs);
    }
    requireEgl(context != EGL_NO_CONTEXT, "OpenGL ES 3.0+ context unavailable");

    surface = eglCreateWindowSurface(display, config, window, nullptr);
    requireEgl(surface != EGL_NO_SURFACE, "EGL create surface failed");
    requireEgl(eglMakeCurrent(display, surface, surface, context), "EGL make current failed");

    requireEgl(eglQuerySurface(display, surface, EGL_WIDTH, &width), "EGL query width failed");
    requireEgl(eglQuerySurface(display, surface, EGL_HEIGHT, &height), "EGL query height failed");
    requireEgl(eglSwapInterval(display, 1), "EGL swap interval failed");

    GLint major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (major < 3) {
        throw std::runtime_error("OpenGL ES 3.0 or newer is required");
    }
    const std::string driver = glText(GL_VENDOR) + " | " + glText(GL_RENDERER) + " | " + glText(GL_VERSION);
    __android_log_print(ANDROID_LOG_INFO, "Arena", "Graphics Driver: %s", driver.c_str());
    GLint sampleBuffers=0, samples=0;
    glGetIntegerv(GL_SAMPLE_BUFFERS,&sampleBuffers); glGetIntegerv(GL_SAMPLES,&samples);
    __android_log_print(ANDROID_LOG_INFO,"Arena","Window MSAA: %d samples",sampleBuffers?samples:0);
    const auto device=graphics::queryGlCapabilities();
    extensions.initialize(device);
    floorTexture=graphics::loadFloor(device,static_cast<graphics::TextureFamily>(std::clamp(texturePreference,0,3))).id;
    frameOutput.initialize(outputPath);
    const std::string shaderVersion = major > 3 || minor >= 2 ? "#version 320 es\n" :
                                      minor >= 1 ? "#version 310 es\n" : "#version 300 es\n";

    pipeline.initialize(shaderVersion);

    if (glGetError() != GL_NO_ERROR) {
        throw std::runtime_error("GLES resource initialization failed");
    }
}

void GlesRenderer::beginFrame(float r, float g, float b) {
    pipeline.begin();
    eglQuerySurface(display, surface, EGL_WIDTH, &width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &height);
    frameOutput.begin(width, height);
    extensions.beginFrame();
    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);

    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

GroundPoint GlesRenderer::screenToGround(float sx, float sy) const noexcept {
    return Camera3D::screenToGround(sx, sy, static_cast<float>(width)/std::max(1,height));
}

void GlesRenderer::drawBox(const Box& box) {
    pipeline.add(box);
}

void GlesRenderer::endFrame() {
    shakePhase=std::fmod(shakePhase+1.f,10000.f);
    pipeline.draw(width,height,floorTexture,std::clamp(cameraShake,0.f,.02f),shakePhase);

    frameOutput.finish();
    extensions.endFrame();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        throw std::runtime_error("GLES draw error " + std::to_string(error));
    }

    requireEgl(eglSwapBuffers(display, surface), "EGL presentation failed; reopen game");
}
