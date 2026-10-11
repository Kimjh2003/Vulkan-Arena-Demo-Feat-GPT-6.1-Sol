#pragma once
#include "astc_gles.h"

// GPU resources belong to the current render-thread context.
// Destroy before releasing that context. GLES 3.1+ compute / 3.0 fragment copy.
class PostProcessor final {
public:
    PostProcessor();
    ~PostProcessor();
    PostProcessor(const PostProcessor&) = delete;
    PostProcessor& operator=(const PostProcessor&) = delete;
    void beginScene(int width, int height);
    void present();
private:
    void initialize();
    void release() noexcept;
    void resizeTargets();
    astc::Capabilities caps;
    GLuint program = 0, vao = 0;
    GLuint sceneFramebuffer = 0, outputFramebuffer = 0;
    GLuint sceneTexture = 0, outputTexture = 0, depthBuffer = 0;
    int width = 0, height = 0, targetWidth = 0, targetHeight = 0;
};
