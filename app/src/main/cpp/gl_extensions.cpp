#include "gl_extensions.h"
#include <EGL/egl.h>
#include <android/log.h>
#include <cstring>

namespace {
template<class T> T load(const char* name) { return reinterpret_cast<T>(eglGetProcAddress(name)); }
void GL_APIENTRY debugMessage(GLenum, GLenum type, GLuint id, GLenum severity,
                             GLsizei length, const GLchar* text, const void*) {
    if(type==GL_DEBUG_TYPE_ERROR_KHR || severity==GL_DEBUG_SEVERITY_HIGH_KHR ||
       severity==GL_DEBUG_SEVERITY_MEDIUM_KHR)
        __android_log_print(ANDROID_LOG_WARN,"ArenaGL","GL message %u: %.*s",id,length,text);
}
}
namespace graphics {
void GlExtensions::initialize() { initialize(queryGlCapabilities()); }
void GlExtensions::initialize(const GlCapabilities& device) {
    const bool debug=device.has("GL_KHR_debug");
    const bool timer=device.has("GL_EXT_disjoint_timer_query");
    __android_log_print(ANDROID_LOG_INFO,"Arena","Extensions: KHR_debug=%s, EXT_disjoint_timer_query=%s",
                       debug?"yes":"no",timer?"yes":"no");
    if(debug) {
        debugCallback=load<PFNGLDEBUGMESSAGECALLBACKKHRPROC>("glDebugMessageCallbackKHR");
        debugControl=load<PFNGLDEBUGMESSAGECONTROLKHRPROC>("glDebugMessageControlKHR");
        if(debugCallback && debugControl) {
            glEnable(GL_DEBUG_OUTPUT_KHR);
            debugCallback(debugMessage,nullptr);
            debugControl(GL_DONT_CARE,GL_DONT_CARE,GL_DEBUG_SEVERITY_NOTIFICATION_KHR,0,nullptr,GL_FALSE);
            __android_log_print(ANDROID_LOG_INFO,"Arena","KHR_debug: driver diagnostics enabled");
        }
    }
    if(!timer) return;
    genQueries=load<PFNGLGENQUERIESEXTPROC>("glGenQueriesEXT");
    deleteQueries=load<PFNGLDELETEQUERIESEXTPROC>("glDeleteQueriesEXT");
    beginQuery=load<PFNGLBEGINQUERYEXTPROC>("glBeginQueryEXT");
    endQuery=load<PFNGLENDQUERYEXTPROC>("glEndQueryEXT");
    getQuery=load<PFNGLGETQUERYIVEXTPROC>("glGetQueryivEXT");
    available=load<PFNGLGETQUERYOBJECTUIVEXTPROC>("glGetQueryObjectuivEXT");
    result=load<PFNGLGETQUERYOBJECTUI64VEXTPROC>("glGetQueryObjectui64vEXT");
    if(!genQueries || !deleteQueries || !beginQuery || !endQuery || !getQuery || !available || !result) return;
    GLint bits=0; getQuery(GL_TIME_ELAPSED_EXT,GL_QUERY_COUNTER_BITS_EXT,&bits);
    if(bits<=0) return;
    genQueries(1,&query);
    GLint disjoint=0; glGetIntegerv(GL_GPU_DISJOINT_EXT,&disjoint);
    __android_log_print(ANDROID_LOG_INFO,"Arena","EXT timer: asynchronous GPU sample every 120 frames");
}
void GlExtensions::beginFrame() {
    if(!query) return;
    GLint disjoint=0;
    if(pending || (frame+1)%120==0) glGetIntegerv(GL_GPU_DISJOINT_EXT,&disjoint);
    if(pending && disjoint) invalid=true;
    if(pending) {
        GLuint ready=0; available(query,GL_QUERY_RESULT_AVAILABLE_EXT,&ready);
        if(ready) {
            if(!invalid) {
                GLuint64 elapsed=0; result(query,GL_QUERY_RESULT_EXT,&elapsed);
                __android_log_print(ANDROID_LOG_INFO,"Arena","GPU scene sample: %.3f ms (not input latency)",
                                   static_cast<double>(elapsed)/1e6);
            }
            pending=false;
        }
    }
    if(++frame%120==0 && !pending) {
        invalid=false; beginQuery(GL_TIME_ELAPSED_EXT,query); active=true;
    }
}
void GlExtensions::endFrame() {
    if(active) { endQuery(GL_TIME_ELAPSED_EXT); active=false; pending=true; }
}
void GlExtensions::reset() noexcept {
    if(active && endQuery) endQuery(GL_TIME_ELAPSED_EXT);
    if(query && deleteQueries) deleteQueries(1,&query);
    if(debugCallback && debugControl) { debugCallback(nullptr,nullptr); glDisable(GL_DEBUG_OUTPUT_KHR); }
    query=0; active=pending=invalid=false; frame=0;
}
}
