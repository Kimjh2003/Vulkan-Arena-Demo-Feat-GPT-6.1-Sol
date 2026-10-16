#include "game_runtime.h"
#include "scene_builder.h"
#include "vulkan_renderer.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

GameRuntime::GameRuntime()=default;
GameRuntime::~GameRuntime()=default;
void GameRuntime::start(ANativeWindow* window,bool manual,int weapon,int preference){
    stop();
    GameSession next;next.reserveRuntimeMemory();next.manualAim=manual;next.combat.equip(weapon);
    auto renderer=std::make_unique<VulkanRenderer>(window,preference);
    std::vector<Box> fixed;SceneBuilder::buildStatic(fixed);renderer->setStaticScene(fixed);
    // Commit only after every initialization/upload succeeds.
    game_=std::move(next);renderer_=std::move(renderer);
#ifndef NDEBUG
    generation_.fetch_add(1,std::memory_order_relaxed);
#endif
}
int GameRuntime::prepare(){
    if(!renderer_)throw std::logic_error("Renderer unavailable");
#ifndef NDEBUG
    const auto before=renderer_->outputGeneration();
#endif
    const bool ready=renderer_->beginFrame(.025f,.035f,.065f);
#ifndef NDEBUG
    if(before!=renderer_->outputGeneration())generation_.fetch_add(1,std::memory_order_relaxed);
#endif
    return ready?0:1;
}
void GameRuntime::frame(const FrameInput& input,void* output){
    if(!renderer_||!output)throw std::invalid_argument("Missing renderer/frame output");
    for(float value:{input.dt,input.moveX,input.moveZ,input.aimX,input.aimZ})
        if(!std::isfinite(value))throw std::invalid_argument("Nonfinite frame input");
    game_.chooseUpgrade(input.upgrade);
    if(input.dash&&!game_.upgrading()&&!game_.gameOver()&&!game_.victory())
        game_.player.dash(input.moveX,input.moveZ);
    game_.updateControls(input.dt,input.moveX,input.moveZ,input.aimX,input.aimZ,input.shooting);
    renderer_->setCameraShake(game_.effects.shake);
    auto stream=renderer_->dynamicScene();
    SceneBuilder::buildDynamic(game_,stream);
    renderer_->endFrame();
#ifndef NDEBUG
    frames_.store(static_cast<std::int64_t>(renderer_->submittedFrames()),std::memory_order_relaxed);
#endif
    FrameState state;state.capture(game_);
    std::memcpy(output,state.values.data(),FrameState::words*sizeof(std::int32_t));
}
void GameRuntime::stop() noexcept {
    renderer_.reset();
#ifndef NDEBUG
    frames_.store(0,std::memory_order_relaxed);
#endif
}
#ifndef NDEBUG
std::int64_t GameRuntime::renderedFrames()const noexcept{return frames_.load(std::memory_order_relaxed);}
std::int64_t GameRuntime::generation()const noexcept{return generation_.load(std::memory_order_relaxed);}
#endif
