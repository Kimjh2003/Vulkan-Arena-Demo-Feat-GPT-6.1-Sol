#pragma once
#include "frame_state.h"
#include <atomic>
#include <memory>

struct ANativeWindow;
class VulkanRenderer;
struct FrameInput {
    float dt,moveX,moveZ,aimX,aimZ;
    bool shooting,dash;
    int upgrade;
};
// Single render-worker owner. Only diagnostic getters may run on the UI thread.
class GameRuntime final {
public:
    GameRuntime();
    ~GameRuntime();
    GameRuntime(const GameRuntime&)=delete;
    GameRuntime& operator=(const GameRuntime&)=delete;
    void start(ANativeWindow* window,bool manual,int weapon,int texturePreference);
    int prepare(); // 0 ready, 1 retry; failures throw to the JNI boundary.
    void frame(const FrameInput& input,void* stateOutput);
    void stop() noexcept;
#ifndef NDEBUG
    std::int64_t renderedFrames() const noexcept;
    std::int64_t generation() const noexcept;
#endif
private:
    std::unique_ptr<VulkanRenderer> renderer_;
    GameSession game_;
#ifndef NDEBUG
    std::atomic<std::int64_t> generation_{0},frames_{0};
#endif
};
static_assert(std::atomic<std::int64_t>::is_always_lock_free);
