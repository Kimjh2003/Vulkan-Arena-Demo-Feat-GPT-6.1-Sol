#include "frame_output.h"
#include <android/log.h>
namespace graphics {
void FrameOutput::initialize(OutputPath path) {
    copy.reset();
    if (path == OutputPath::DiagnosticCopy) copy = std::make_unique<PostProcessor>();
    __android_log_print(ANDROID_LOG_INFO, "Arena", "Frame output: %s",
        copy ? "diagnostic copy" : "direct; no offscreen targets or copy passes");
}
void FrameOutput::begin(int width, int height) {
    if (copy) copy->beginScene(width, height);
    else glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void FrameOutput::finish() { if (copy) copy->present(); }
}
