#pragma once
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>
#include <android/native_window.h>
#include <cstddef>
#include <string>
#include <vector>
#include "render_types.h"
#include "camera3d.h"
#include "frame_output.h"
#include "gl_extensions.h"
#include "scene_pipeline.h"
#include <memory>

// Construct, call, and destroy on the same render thread. Owns an acquired window reference.
class GlesRenderer final {
public:
    explicit GlesRenderer(ANativeWindow* window, graphics::OutputPath path = graphics::OutputPath::Direct, int texturePreference=0);
    ~GlesRenderer();

    GlesRenderer(const GlesRenderer&) = delete;
    GlesRenderer& operator=(const GlesRenderer&) = delete;
    GlesRenderer(GlesRenderer&&) = delete;
    GlesRenderer& operator=(GlesRenderer&&) = delete;

    void beginFrame(float r, float g, float b);
    void drawBox(const Box& box);
    void endFrame();
    GroundPoint screenToGround(float sx, float sy) const noexcept;
    void setCameraShake(float amount) noexcept { cameraShake = amount; }

    int getWidth() const noexcept { return width; }
    int getHeight() const noexcept { return height; }

private:
    void initialize();
    void release() noexcept;

    ANativeWindow* window = nullptr;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    float cameraShake = 0, shakePhase = 0;
    GLuint floorTexture = 0;
    graphics::FrameOutput frameOutput;
    graphics::GlExtensions extensions;
    graphics::OutputPath outputPath;
    int texturePreference=0;
    int width = 0;
    int height = 0;
    graphics::ScenePipeline pipeline;
};
