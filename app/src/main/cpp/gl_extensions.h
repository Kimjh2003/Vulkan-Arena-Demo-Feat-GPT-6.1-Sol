#pragma once
#include <GLES3/gl32.h>
#include <GLES2/gl2ext.h>
#include "gl_capabilities.h"

namespace graphics {
// Optional extensions are loaded only after a current context advertises them.
// One sparse query; never wait for the GPU and discard disjoint intervals.
class GlExtensions final {
public:
    void initialize();
    void initialize(const GlCapabilities&);
    void beginFrame();
    void endFrame();
    void reset() noexcept;
private:
    PFNGLDEBUGMESSAGECALLBACKKHRPROC debugCallback = nullptr;
    PFNGLDEBUGMESSAGECONTROLKHRPROC debugControl = nullptr;
    PFNGLGENQUERIESEXTPROC genQueries = nullptr;
    PFNGLDELETEQUERIESEXTPROC deleteQueries = nullptr;
    PFNGLBEGINQUERYEXTPROC beginQuery = nullptr;
    PFNGLENDQUERYEXTPROC endQuery = nullptr;
    PFNGLGETQUERYIVEXTPROC getQuery = nullptr;
    PFNGLGETQUERYOBJECTUIVEXTPROC available = nullptr;
    PFNGLGETQUERYOBJECTUI64VEXTPROC result = nullptr;
    GLuint query = 0;
    unsigned frame = 0;
    bool active = false, pending = false, invalid = false;
};
}
