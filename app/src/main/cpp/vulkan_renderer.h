#pragma once
#include <android/native_window.h>
#include <memory>
#include <vector>
#include "render_types.h"
#include "scene_instance_writer.h"

// Android WSI and two reusable frame slots. All calls use the render thread.
class VulkanRenderer final {
public:
    VulkanRenderer(ANativeWindow* window,int texturePreference=0);
    ~VulkanRenderer();
    VulkanRenderer(const VulkanRenderer&)=delete;
    VulkanRenderer& operator=(const VulkanRenderer&)=delete;
    bool beginFrame(float r,float g,float b);
    SceneInstanceWriter dynamicScene();
    void setStaticScene(const std::vector<Box>& boxes);
    void endFrame();
    void setCameraShake(float amount) noexcept;
    unsigned long long submittedFrames() const noexcept;
    unsigned long long outputGeneration() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
