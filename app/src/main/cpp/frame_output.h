#pragma once
#include "post_processor.h"
#include <memory>
namespace graphics {
enum class OutputPath { Direct, DiagnosticCopy };
// Current GL context owns these resources; reset before EGL context destruction.
// Direct allocates no offscreen targets, shaders or fullscreen copy passes.
class FrameOutput final {
public:
    void initialize(OutputPath path = OutputPath::Direct);
    void begin(int width, int height);
    void finish();
    void reset() noexcept { copy.reset(); }
private:
    std::unique_ptr<PostProcessor> copy;
};
}
