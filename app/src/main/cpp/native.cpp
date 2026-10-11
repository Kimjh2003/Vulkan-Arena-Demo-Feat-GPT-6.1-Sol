#include <android/native_window_jni.h>
#include <jni.h>
#include <android/log.h>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include "gameplay/game_session.h"
#include "scene_builder.h"
#include "hud_presenter.h"
#include "gles_renderer.h"

static std::mutex g_mutex;
static std::unique_ptr<GlesRenderer> g_renderer;
static GameSession g_game;
static std::vector<Box> g_sceneCache;

extern "C" JNIEXPORT jstring JNICALL
Java_dev_demo_arena_MainActivity_start(JNIEnv* env, jobject, jobject surface,jboolean manual,jint weapon,jint texturePreference) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_renderer.reset();
    g_game = GameSession{};
    g_game.manualAim=manual;g_game.combat.equip(weapon);
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    try {
        g_renderer = std::make_unique<GlesRenderer>(window,graphics::OutputPath::Direct,texturePreference);
        ANativeWindow_release(window);
        return env->NewStringUTF("");
    } catch (const std::exception& e) {
        if (window) {
            ANativeWindow_release(window);
        }
        __android_log_print(ANDROID_LOG_ERROR, "Arena", "Initialization error: %s", e.what());
        return env->NewStringUTF("Unable to start the game. Back to retry.");
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_dev_demo_arena_MainActivity_frame(JNIEnv* env, jobject,jfloat dt,jfloat moveX,jfloat moveZ,
                                      jfloat aimX,jfloat aimZ,jboolean shooting,jboolean dash,jint upgrade) {
    std::lock_guard<std::mutex> lock(g_mutex);
    try {
        if (!g_renderer) {
            throw std::runtime_error("Renderer unavailable");
        }
        g_game.chooseUpgrade(upgrade);
        if(dash && !g_game.upgrading() && !g_game.gameOver() && !g_game.victory()) g_game.player.dash(moveX,moveZ);
        g_game.updateControls(dt,moveX,moveZ,aimX,aimZ,shooting);
        g_renderer->setCameraShake(g_game.effects.shake);
        g_renderer->beginFrame(0.025f, 0.035f, 0.065f);
        SceneBuilder::build(g_game,g_sceneCache);
        for (const auto& box : g_sceneCache) {
            g_renderer->drawBox(box);
        }
        g_renderer->endFrame();

        const auto text = HudPresenter::state(g_game)+"|"+std::to_string(g_game.effects.events);
        return env->NewStringUTF(text.c_str());
    } catch (const std::exception& e) {
        __android_log_print(ANDROID_LOG_ERROR, "Arena", "Frame error: %s", e.what());
        return env->NewStringUTF("ERROR: Game interrupted. Back to retry.");
    }
}

extern "C" JNIEXPORT void JNICALL
Java_dev_demo_arena_MainActivity_stop(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_renderer.reset();
}
