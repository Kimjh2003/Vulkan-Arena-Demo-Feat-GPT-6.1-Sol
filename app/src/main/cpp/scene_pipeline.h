#pragma once
#include <GLES3/gl32.h>
#include <vector>
#include <string>
#include "render_types.h"
namespace graphics {
// Reusable GPU box mesh + streamed per-instance state, depth shadow pass and UBO.
// All methods, including reset, require the owning current GL context.
class ScenePipeline final {
public:
    void initialize(const std::string& shaderVersion);
    void begin() { instances.clear(); }
    void add(const Box&);
    void draw(int width,int height,GLuint floorTexture,float shake,float phase);
    void reset() noexcept;
private:
    struct Instance {
        float origin[3],extent[3],color[3],rotation[2],material[2];
    };
    static_assert(sizeof(Instance)==52,"Packed instance attributes");
    GLuint program=0,vao=0,mesh=0,stream=0,uniforms=0;
    GLuint shadowFramebuffer=0,shadowTexture=0;
    GLint passLocation=-1,shadowLocation=-1;
    size_t capacity=0;
    int shadowSize=1024;
    std::vector<Instance> instances;
};
}
