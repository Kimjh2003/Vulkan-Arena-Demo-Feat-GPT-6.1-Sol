#include <android/native_window_jni.h>
#include <android/log.h>
#include <jni.h>
#include <memory>
#include <stdexcept>
#include "game_runtime.h"

namespace {
GameRuntime runtime;
struct ReleaseWindow {
    void operator()(ANativeWindow* window)const noexcept {if(window)ANativeWindow_release(window);}
};
template<class Action>jint status(const char* operation,Action&& action)noexcept {
    try {return action();}
    catch(const std::exception& error){
        __android_log_print(ANDROID_LOG_ERROR,"ArenaVulkan","%s: %s",operation,error.what());
    }catch(...){__android_log_print(ANDROID_LOG_ERROR,"ArenaVulkan","%s: unknown native failure",operation);}
    return -1;
}
}
extern "C" JNIEXPORT jstring JNICALL
Java_dev_demo_arena_NativeBridge_start(JNIEnv* env,jobject,jobject surface,jboolean manual,jint weapon,jint texturePreference){
    const auto result=status("Initialization",[&]{
        if(!surface)throw std::invalid_argument("Missing Android surface");
        std::unique_ptr<ANativeWindow,ReleaseWindow> window(ANativeWindow_fromSurface(env,surface));
        runtime.start(window.get(),manual,weapon,texturePreference);
        return 0;
    });
    if(env->ExceptionCheck())return nullptr;
    return env->NewStringUTF(result==0?"":"Unable to start the game. Back to retry.");
}
extern "C" JNIEXPORT jint JNICALL
Java_dev_demo_arena_NativeBridge_prepare(JNIEnv*,jobject){
    return status("Prepare",[]{return runtime.prepare();});
}
extern "C" JNIEXPORT jint JNICALL
Java_dev_demo_arena_NativeBridge_frame(JNIEnv* env,jobject,jfloat dt,jfloat moveX,jfloat moveZ,
    jfloat aimX,jfloat aimZ,jboolean shooting,jboolean dash,jint upgrade,jobject stateBuffer){
    return status("Frame",[&]{
        if(!stateBuffer)throw std::invalid_argument("Missing direct frame-state buffer");
        void* output=env->GetDirectBufferAddress(stateBuffer);
        if(!output||env->GetDirectBufferCapacity(stateBuffer)<static_cast<jlong>(FrameState::words*sizeof(std::int32_t)))
            throw std::invalid_argument("Invalid direct frame-state buffer");
        runtime.frame(FrameInput{dt,moveX,moveZ,aimX,aimZ,bool(shooting),bool(dash),upgrade},output);
        return 0;
    });
}
extern "C" JNIEXPORT void JNICALL
Java_dev_demo_arena_NativeBridge_stop(JNIEnv*,jobject){runtime.stop();}
#ifndef NDEBUG
extern "C" JNIEXPORT jlong JNICALL
Java_dev_demo_arena_NativeDiagnostics_renderedFrames(JNIEnv*,jobject){return runtime.renderedFrames();}
extern "C" JNIEXPORT jlong JNICALL
Java_dev_demo_arena_NativeDiagnostics_rendererGeneration(JNIEnv*,jobject){return runtime.generation();}
#endif
